/**************************************************************************
**   Copyright (C): 2018
**   Author: Navid MAHMOUDIAN BIDGOLI <navid.mahmoudian-bidgoli@inria.fr>
**   Supervisors: Aline ROUMY, Thomas MAUGEY
**   Inria research institute
**************************************************************************/
#include "vtm_utility.h"
#include "ICOV_DistorsionCost.h"
#include "ICOV_Quant.h"
#include "ICOV_Transform2d.h"
#include "icov_exception.h"
#include "icov_log.h"
#include <algorithm>
#include <limits>

Area &VTM_Utility::operator&=(Area &a, const Area &b) {
  Int x1 = std::max(a.x, b.x);
  Int y1 = std::max(a.y, b.y);
  a.width = std::min(a.x + a.width, b.x + b.width) - x1;
  a.height = std::min(a.y + a.height, b.y + b.height) - y1;
  a.x = x1;
  a.y = y1;
  if (a.width <= 0 || a.height <= 0)
    a = Area();
  return a;
}

Area VTM_Utility::operator&(const Area &a, const Area &b) {
  Area c = a;
  return c &= b;
}

void VTM_Utility::positionRelativeTo(UnitArea &inputOutput,
                                     const Position &origin) {
  for (UInt i = 0; i < inputOutput.blocks.size(); i++) {
    inputOutput.blocks[i].relativeTo(origin);
  }
}

// int dx_snake[4] = {1,-1, 0, 0};
// int dy_snake[4] = {0, 0, -1, 1};
static const int dx[4] = {1, -1, 0, 0};
static const int dy[4] = {0, 0, -1, 1};
void dfs_360(AreaBuf<bool> &inputOutput,
             std::vector<Position> &currentComponentBlocks, int x, int y) {
  currentComponentBlocks.push_back(Position(x, y));
  inputOutput.at(x, y) = false;
  int iWidth = int(
      inputOutput
          .width); // if we do not convert to int, the traversing equation for
                   // 360 video (neighbor_x = (iWidth + (neighbor_x%iWidth)) %
                   // iWidth) converts everything to UInt
  int iHeight = int(inputOutput.height);
  for (int i = 0; i < 4; i++) {
    int neighbor_x = x + dx[i];
    neighbor_x =
        (iWidth + (neighbor_x % iWidth)) %
        iWidth; // to generate circular traversing for 360 video in x direction
    int neighbor_y = y + dy[i];
    if (neighbor_y < 0 ||
        neighbor_y >= iHeight) // do not check outside image in y direction
      continue;
    if (inputOutput.at(neighbor_x, neighbor_y))
      dfs_360(inputOutput, currentComponentBlocks, neighbor_x, neighbor_y);
  }
}

void VTM_Utility::quantizeFrame(const int qp, const PelUnitBuf &originalImage, PelUnitBuf &quantizedImage,
                                icov::video::EncodingStructure &encStruct, const int bitDepths[MAX_NUM_CHANNEL_TYPE],
                                int maxLog2TrDynamicRange)
{
  if (originalImage.chromaFormat != quantizedImage.chromaFormat)
    throw std::runtime_error("The original image and the quantized image "
                             "should have similar chroma");

  const UInt numCh = getNumberValidComponents(originalImage.chromaFormat);

  for (UInt ch = 0; ch < numCh; ch++) {
    const ComponentID compID = ComponentID(ch);
    VTM_Utility::quantizeFrame(
        qp, originalImage, quantizedImage, encStruct, compID, bitDepths,
        maxLog2TrDynamicRange); // give the whole extended image ( .getOrigBuf()
                                // and .getRecoBuf() ) to the function because
                                // vector icues_enc is based on extended frames
  }
}

void VTM_Utility::quantizeFrame(const int qp, const PelUnitBuf &originalImage,
                                PelUnitBuf &quantizedImage,
                                icov::video::EncodingStructure &encStruct,
                                const ComponentID compID,
                                const int bitDepths[MAX_NUM_CHANNEL_TYPE],
                                int maxLog2TrDynamicRange) {
  ChannelType channelType = toChannelType(compID);
  int bit_depth = bitDepths[channelType];
  ChromaFormat chFrmt = encStruct.getICU(0).get_sourceX().chromaFormat;

  icov::video::QpParam cQP(qp, channelType, chFrmt, bit_depth, 0, 0);

  ICOV_DEBUG << ICOV_DEBUG_HEADER("quantizeFrame ") << " ch=" << (int)compID
             << " format " << (int)chFrmt << ", qp=" << cQP.Qp
             << ", bd=" << bit_depth << ", ml=" << maxLog2TrDynamicRange;

  icov::video::Transform2D transform2D;
  icov::video::Quant quant;
  TCoeff uiAbsSum;

  // FOR EACH BLOCK (ICU) PARALLELIZATION DOES NOT WORK
  //#pragma omp parallel
  for (UInt i = 0; i < encStruct.vec_icus().size(); i++)
  {
    icov::video::InteractiveCodingUnit &curr_icu =
        encStruct.getICU(i).get_sourceX();

    // FOR EACH TU IN THE BLOCK
    for (UInt j = 0; j < curr_icu.TUs.size(); j++) {
      icov::video::TransformUnit &curr_tu = curr_icu.TUs[j];

      curr_tu.save();

      // DCT
      transform2D.transformNxN(curr_tu.getTransformCoeffs(compID),
                               originalImage, curr_tu.block(compID), bit_depth,
                               maxLog2TrDynamicRange);

      // Q
      quant.quant(curr_tu.getQuantizedTransformCoeffs(compID),
                  curr_tu.getTransformCoeffs(compID), uiAbsSum, cQP,
                  SliceType::I_SLICE, bit_depth, maxLog2TrDynamicRange);

      // iQ
      quant.dequant(curr_tu.getInverseQuantizedTransformCoeffs(compID),
                    curr_tu.getQuantizedTransformCoeffs(compID), cQP, bit_depth,
                    maxLog2TrDynamicRange);

      // iDCT
      transform2D.invTransformNxN(
          quantizedImage, curr_tu.block(compID),
          curr_tu.getInverseQuantizedTransformCoeffs(compID), bit_depth,
          maxLog2TrDynamicRange);

      //            totalDist +=
      //            distCost.getDistPart(VTM_Utility::getAreaBuf(originalImage,
      //            curr_tu.block(compID)),
      //                                              VTM_Utility::getAreaBuf(quantizedImage,
      //                                              curr_tu.block(compID)),
      //                                              bit_depth, compID,
      //                                              DF_SSE);
    }
  }
}

double VTM_Utility::
    computeTheoreticalRateWithEntropyForCompressPrecomputeFrequencyTable2ByteValuesAdaptiveRangeAbsoluteValues(
        std::vector<int> &in, bool encodeFrequencyTableInBitStream,
        int maxLog2DynamicRange) {

  const int requiredBitsToEncodeNumBitsNeededToEncodeFrequencyTableVal =
      4; // ArithmeticCoding_Utility::requiredBitsToEncodeNumBitsNeededToEncodeFrequencyTableVal;
  const int entropyCodingMaximumAbs = (1 << maxLog2DynamicRange);
  uint32_t n_range = entropyCodingMaximumAbs + 1;

  uint32_t maximumOccurance = 0;
  uint32_t lastAbsPosition = 0;

  std::map<uint32_t, UInt> freqTable;
  for (unsigned int i = 0; i < in.size(); i++) {
    int b = abs(in[i]);
    if (b == EOF)
      break;
    if (b < 0 || b >= n_range)
      throw std::logic_error("Assertion error");

    uint32_t position = static_cast<uint32_t>(b);
    freqTable[position]++;

    if (freqTable[position] > maximumOccurance)
      maximumOccurance = freqTable[position];
    if (position > lastAbsPosition)
      lastAbsPosition = position;
  }
  n_range = lastAbsPosition + 1; // new range
  freqTable[n_range]++;

  int n_bits2EncodePosition = std::ceil(std::log2(n_range));

  ICOV_ASSERT_MSG(maximumOccurance > 0,
                  "maximumOccurance=" + std::to_string(maximumOccurance));

  uint32_t n_bitsPerFrequencyTableVal =
      maximumOccurance > 0 ? std::ceil(std::log2(maximumOccurance)) : 0;
  if (n_bitsPerFrequencyTableVal == 0) // to handle the case log(1)=0
    n_bitsPerFrequencyTableVal = 1;

  if (n_bitsPerFrequencyTableVal >
      (1 << requiredBitsToEncodeNumBitsNeededToEncodeFrequencyTableVal))
    throw std::invalid_argument(
        "Error: can not compress. maximum occurance of values is ...");

  uint32_t requiredBitsToEncodeFrequencyTable = 0;

  if (encodeFrequencyTableInBitStream) {
    // TODO: I haven't check this function when encodeFrequencyTableInBitStream
    // is true

    // step 1: the first 3 (more precisely
    // ArithmeticCoding_Utility::requiredBitsToEncodeNumBitsNeededToEncodeFrequencyTableVal)
    // bits is used to encode requiredBitsPerFrequencyTableVal
    requiredBitsToEncodeFrequencyTable +=
        requiredBitsToEncodeNumBitsNeededToEncodeFrequencyTableVal;

    // step 2: the second maxLog2TrDynamicRange goes to encode maxAbsValue
    requiredBitsToEncodeFrequencyTable += maxLog2DynamicRange + 1;

    // step 3: Write frequency table
    // it will be written in the next loop over frequency when we are computing
    // probabilities
  }

  UInt n_elem =
      in.size() + 1; // +1 is for eof, (freqTable[n_range]++; few lines above)

  double entropy = 0.;
  double f, p_x;

  for (auto const &x : freqTable) {
    if (encodeFrequencyTableInBitStream) {
      requiredBitsToEncodeFrequencyTable += n_bits2EncodePosition;
      requiredBitsToEncodeFrequencyTable += n_bitsPerFrequencyTableVal;
    }

    p_x = double(x.second) / double(n_elem);
    f = p_x * std::log2(p_x);
    //        if(std::isinf(f) && (fabs(pVal) < 1e-13) ) // to solve the problem
    //        of "epsilon * log2(number/epsilon)"
    //            f = 0.;
    if (f != f) // if it is nan make it zero
      f = 0.;
    entropy -= f;
  }

  double transmissionBits =
      double(n_elem) * entropy + double(in.size()) +
      double(
          requiredBitsToEncodeFrequencyTable); // in.size() because of encoding
                                               // sign of absolute values
  return transmissionBits;
}

void VTM_Utility::copyBordersFor360(PelUnitBuf &mainFrame_roi,
                                    const ComponentID compID) {
  const auto &compBuf = mainFrame_roi.get(compID);

  Pel *src1 = compBuf.buf;
  Pel *dst1 = compBuf.buf + compBuf.width;

  Pel *src2 = compBuf.buf + compBuf.width - 1;
  Pel *dst2 = compBuf.buf - 1;

  for (UInt y = 0; y < compBuf.height; y++, src1 += compBuf.stride,
            dst1 += compBuf.stride, src2 += compBuf.stride,
            dst2 += compBuf.stride) {
    *dst1 = *src1;
    *dst2 = *src2;
  }
}

// ldpc_codec_layer::BitSymbolProvider instantiateBitSymbolProvider(
//     const ComponentID id, const CCoeffBuf &si_tuCoeff,
//     const ldpc_codec_layer::QaryDistribution::quantization_params
//     &bsp_params) const {
//   auto x_tuCoeff = this->getQuantizedTransformCoeffs(id);
//   size_t tu_size = x_tuCoeff.area();

//  ldpc_codec_layer::BitSymbolProvider bsp(
//      SymboleVec(x_tuCoeff.buf, x_tuCoeff.buf + tu_size));

//  ICOV_ASSERT_MSG(
//      (size_t)si_tuCoeff.area() == tu_size,
//      "Transform units from Source and its SI must be the same size: " +
//          std::to_string(tu_size));

//  bsp.set_si_and_compute_pz_params(
//      SIVec(si_tuCoeff.buf, si_tuCoeff.buf + tu_size), bsp_params);

//  return bsp;
//}
