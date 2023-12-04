#include "Encodingstructure.h"

#include "ICOV_Quant.h"
#include "ICOV_Transform2d.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "vtm_utility.h"
#include <opencv2/imgproc.hpp>

////////////////////////////////////////////////////////////////////
icov::video::EncodingStructure::EncodingStructure() { deallocate(); }

icov::video::EncodingStructure::EncodingStructure(
    const std::vector<IcuEncoder> &icus, const UInt icusPerRow,
    const UInt icusPerColumn) {
  set(icus, icusPerRow, icusPerColumn);
}

icov::video::EncodingStructure::~EncodingStructure() { deallocate(); }

////////////////////////////////////////////////////////////////////
void icov::video::EncodingStructure::set(const std::vector<IcuEncoder> &icus,
                                         const UInt icusPerRow,
                                         const UInt icusPerColumn) {
  deallocate();

  _vec_icus = icus;
  _n_icusPerRow = icusPerRow;
  _n_icusPerCol = icusPerColumn;
  CHECK(_vec_icus.size() != _n_icusPerRow * _n_icusPerCol,
        "incorrect number of icus per row and per column");

  bool *reqPtr = (bool *)xMalloc(bool, _n_icusPerRow *_n_icusPerCol);
  _requestedICUs =
      AreaBuf<bool>(reqPtr, _n_icusPerRow, _n_icusPerRow, _n_icusPerCol);
  bool *prePtr = (bool *)xMalloc(bool, _n_icusPerRow *_n_icusPerCol);
  _decodedICUs =
      AreaBuf<bool>(prePtr, _n_icusPerRow, _n_icusPerRow, _n_icusPerCol);

  UInt n_icus = _n_icusPerRow * _n_icusPerCol;
  _vec_pixelsRequestedPerICU.resize(n_icus);

  _vec_decodedICUs.reserve(n_icus);

  _vec_tmp_alreadyProcessedICUs.reserve(n_icus);
  bool *alreadyProPtr = (bool *)xMalloc(bool, _n_icusPerRow *_n_icusPerCol);
  _tmp_alreadyProcessedICUs =
      AreaBuf<bool>(alreadyProPtr, _n_icusPerRow, _n_icusPerRow, _n_icusPerCol);
}


////////////////////////////////////////////////////////////////////
Position
icov::video::EncodingStructure::get_icuPosition(const UInt icu_index) const {
  div_t divresult;
  divresult = div(icu_index, Int(_n_icusPerRow));
  UInt icu_x = divresult.rem;
  UInt icu_y = divresult.quot;
  Position block_position(icu_x, icu_y);

  if (this->get_icuIndex(block_position) != icu_index)
    throw std::logic_error(
        "The output block position is not correct. Something is missing");

  return block_position;
}

UInt icov::video::EncodingStructure::get_icuIndex(
    const omnidirectional_layer::Block &b) const {
  int x_block = b.parent()->col(b.u());
  int y_block = b.parent()->row(b.v());

  int w_block = b.parent()->col(b.w());
  int h_block = b.parent()->row(b.h());

  std::cout << x_block << ", " << y_block << ", " << w_block << ", " << h_block;

  Position pos(static_cast<float>(x_block) / static_cast<float>(w_block),
               static_cast<float>(y_block) / static_cast<float>(h_block));

  return get_icuIndex(pos);
}

////////////////////////////////////////////////////////////////////
void icov::video::EncodingStructure::
    create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
        ChromaFormat chFmt, const UInt sourceWidth_luma,
        const UInt sourceHeight_luma, const UInt sourceMargin_x_luma,
        const UInt icuWidth_luma, const UInt icuHeight_luma,
        const UInt tuWidth_luma, const UInt tuHeight_luma) {
  using namespace VTM_Utility;
  const bool si_createTransformBuffer = true;
  const bool si_createQuantizedTransformBuffer = true;
  const bool si_createInverseQuantizedTransformBuffer = true;

  std::map<int, icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE>
      mapOfSidesCondition;
  mapOfSidesCondition[0] = omnidirectional_layer::BlockPrediction::LEFT;
  mapOfSidesCondition[1] = omnidirectional_layer::BlockPrediction::RIGHT;
  mapOfSidesCondition[2] = omnidirectional_layer::BlockPrediction::TOP;
  mapOfSidesCondition[3] = omnidirectional_layer::BlockPrediction::BOTTOM;
  mapOfSidesCondition[4] = omnidirectional_layer::BlockPrediction::TOP_LEFT;
  mapOfSidesCondition[5] = omnidirectional_layer::BlockPrediction::BOTTOM_LEFT;
  mapOfSidesCondition[6] = omnidirectional_layer::BlockPrediction::BOTTOM_RIGHT;
  mapOfSidesCondition[7] = omnidirectional_layer::BlockPrediction::TOP_RIGHT;

  unsigned int mapSize = mapOfSidesCondition.size();

  UInt nx_icus = (sourceWidth_luma + icuWidth_luma - 1) / icuWidth_luma;
  UInt ny_icus = (sourceHeight_luma + icuHeight_luma - 1) / icuHeight_luma;
  UInt n_icus = nx_icus * ny_icus;

  std::vector<IcuEncoder> icus_enc;

  icus_enc.reserve(n_icus);
  div_t divresult;

  Area extendedFrameArea_luma(0, 0, sourceWidth_luma + 2 * sourceMargin_x_luma,
                              sourceHeight_luma);
  for (UInt icuIndex = 0; icuIndex < n_icus; icuIndex++) {
    divresult = div(Int(icuIndex), Int(nx_icus));
    UInt icuXPosTopLeft = divresult.rem;
    UInt icuYPosTopLeft = divresult.quot;

    ICOV_TRACE << "create ICU #" << icuIndex << " (" << icuXPosTopLeft << ", "
               << icuYPosTopLeft << ")";

    Area icuLumaArea =
        Area(icuXPosTopLeft * icuWidth_luma + sourceMargin_x_luma,
             icuYPosTopLeft * icuHeight_luma, icuWidth_luma, icuHeight_luma) &
        extendedFrameArea_luma;

    try {
      CHECK(static_cast<UInt>(icuLumaArea.width) != icuWidth_luma ||
                static_cast<UInt>(icuLumaArea.height) != icuHeight_luma,
            "currently only frame which is divisible by icuWidth is supported");
    } catch (...) {
      std::cout << "...\n";
      return;
    }

    icus_enc.push_back(IcuEncoder(chFmt, icuLumaArea));
    IcuEncoder &icu_enc = icus_enc.back();
    icu_enc.createTUs(tuWidth_luma, tuHeight_luma);

    using SI_SIDE = icov::video::IntraPrediction::REFERENCE_SAMPLE_SIDE;

    for (UInt i = 0; i < mapSize; i++) {
      SI_SIDE refSamp = mapOfSidesCondition[i];

      if (
          // TOP
          !(icuYPosTopLeft == 0 && (refSamp & SI_SIDE::TOP)) &&
          // BOTTOM
          !(icuYPosTopLeft == (ny_icus - 1) && (refSamp & SI_SIDE::BOTTOM))

      ) {

        icu_enc.addIntraSI(refSamp, si_createTransformBuffer,
                           si_createQuantizedTransformBuffer,
                           si_createInverseQuantizedTransformBuffer);
      }
    }
  }
  set(icus_enc, nx_icus, ny_icus);
}

////////////////////////////////////////////////////////////////////
void icov::video::EncodingStructure::applyPredictionBasedOnEntropy(
    icov::video::BufGroups &encpic, const ComponentID compID,
    const int bitDepths[MAX_NUM_CHANNEL_TYPE], const UInt tuWidth_luma,
    const UInt tuHeight_luma, bool encodeFrequencyTableInBitStream,
    const int qp, int maxLog2TrDynamicRange) {

  const auto reconstructionUnitBuffer = encpic.getBuf(PIC_RECONSTRUCTION);
  const ChromaFormat chromaFormatIDC = getICU(0).get_sourceX().chromaFormat;

  const UInt tuWidth_component = std::max<UInt>(
      tuWidth_luma >> getComponentScaleX(compID, chromaFormatIDC), 1);

  const UInt tuHeight_component = std::max<UInt>(
      tuHeight_luma >> getComponentScaleY(compID, chromaFormatIDC), 1);

  const UInt nBands = tuWidth_component * tuHeight_component;

  const ChannelType chType = toChannelType(compID);

  int processed = 0;
  int max_count = _vec_icus.size();
#pragma omp parallel for
  for (UInt i = 0; i < max_count; i++) {

    icov::video::IntraPrediction intraPred;
    intraPred.init(chromaFormatIDC);

    std::vector<TCoeff> transformSequence_sourceX, transformSequence_sourceY;
    std::vector<int> residual;
    Transform2D transform2D;
    Quant quant;
    TCoeff uiAbsSum;
    QpParam cQP(qp, toChannelType(compID), getICU(0).get_sourceX().chromaFormat,
                bitDepths[chType], 0, 0);

    std::vector<UInt> availableDirModes;
    if (chType == CHANNEL_TYPE_LUMA) {
      availableDirModes.reserve(NUM_LUMA_MODE);
      for (UInt dirMode = 0; dirMode < NUM_LUMA_MODE; dirMode++)
        availableDirModes.push_back(dirMode);
    } else
      availableDirModes = {DM_CHROMA_IDX, PLANAR_IDX, DC_IDX, HOR_IDX, VER_IDX};

    int progress = (processed * 100) / (max_count - 1);

#pragma omp atomic
    processed++;

    std::cout << "    " << progress << "%                \r" << std::flush;
    IcuEncoder &icu_enc = _vec_icus[i];
    std::map<omnidirectional_layer::BlockPrediction::ePredType,
             SideInformationUnit> &currICUintraMap = icu_enc.get_intraSIsMap();

    const CompArea &compArea = icu_enc.get_sourceX().block(compID);

    // Iterate through all elements in std::map
    for (auto it = currICUintraMap.begin(); it != currICUintraMap.end();
         ++it) // loop over all intra side information
    {
      auto &si = it->second;
      const auto &refSampSide = it->first;

      uint picId = PIC_PREDICTION + refSampSide;
      std::cout << "---- pred pic is " << refSampSide << std::endl;
      const PelUnitBuf predictionUnitBuf = encpic.get(picId);

      //      uint picId = refSampSide;
      //      std::cout << "---- pred pic is " << picId << std::endl;

      PelBuf roiComp_predictionFrameBuff =
          VTM_Utility::getAreaBuf(predictionUnitBuf, compArea);

      auto &si_TUs = si.TUs;
      unsigned int n_tus = si_TUs.size();

      if (chType == CHANNEL_TYPE_CHROMA)
        availableDirModes[0] =
            si.intraDir[COMPONENT_Y]; // set the direct mode for current SI
                                      // chroma channel

      intraPred.initIntraPatternChType(reconstructionUnitBuffer, refSampSide,
                                       compArea, bitDepths[chType], false);

      double entropy = std::numeric_limits<double>::max();

      // SEARCH BEST DIRMODE FOR INTRA PRED
      for (const UInt &dirMode : availableDirModes) // access by const reference
      {

        intraPred.predIntraAng(roiComp_predictionFrameBuff, dirMode);
        for (UInt j = 0; j < n_tus; j++) {
          TransformUnit &curr_tu_si = si_TUs[j];

          transform2D.transformNxN(curr_tu_si.getTransformCoeffs(compID),
                                   predictionUnitBuf, curr_tu_si.block(compID),
                                   bitDepths[chType], maxLog2TrDynamicRange);

          quant.quant(curr_tu_si.getQuantizedTransformCoeffs(compID),
                      curr_tu_si.getTransformCoeffs(compID), uiAbsSum, cQP,
                      SliceType::I_SLICE, bitDepths[chType],
                      maxLog2TrDynamicRange);
          //                    quant.dequant(curr_tu_si.getInverseQuantizedTransformCoeffs(compID),
          //                    curr_tu_si.getQuantizedTransformCoeffs(compID),
          //                    cQP, bitDepths[chType], maxLog2TrDynamicRange);

          //                    cout << "icu " << i << ", comID " << compID <<
          //                    ", refSampl " << refSamplToStr(refSampSide) <<
          //                    ", [x, y, w, h] = [ "<<
          //                    curr_tu_si.block(compID).x << ", " <<
          //                    curr_tu_si.block(compID).y << ", " <<
          //                    curr_tu_si.block(compID).width << ", " <<
          //                    curr_tu_si.block(compID).height << "]" << ",
          //                    dirMode= " << dirMode << endl;
        }
        double currEntropy =
            static_cast<double>(si.estimateSIRequiredBitsToCodeIntraMode(
                compID, dirMode, refSampSide));
        residual.clear();

        for (UInt b = 0; b < nBands; b++) {
          icu_enc.getTransformSequences(
              compID, refSampSide, b, tuWidth_component, tuHeight_component,
              transformSequence_sourceX, transformSequence_sourceY);

          //                    if(i==0 && dirMode==67)currEntropy
          //                    {
          //                        cout << "transformSequence_sourceY[j]= ";
          //                        for(unsigned int j=0; j <
          //                        transformSequence_sourceY.size(); j++)
          //                            cout << transformSequence_sourceY[j] <<
          //                            ", ";
          //                        cout << endl;
          //                    }

          //                    if(i==0 && dirMode==67)
          //                    {
          //                        cout << "transformSequence_sourceX[j]= ";
          //                        for(unsigned int j=0; j <
          //                        transformSequence_sourceX.size(); j++)
          //                            cout << transformSequence_sourceX[j] <<
          //                            ", ";
          //                        cout << endl;
          //                    }

          CHECK(transformSequence_sourceX.size() !=
                    transformSequence_sourceY.size(),
                "transform sequence vector size do not match");
          for (unsigned int j = 0; j < transformSequence_sourceX.size(); j++) {
            residual.push_back(transformSequence_sourceX[j] -
                               transformSequence_sourceY[j]);
          }

          currEntropy += VTM_Utility::
              computeTheoreticalRateWithEntropyForCompressPrecomputeFrequencyTable2ByteValuesAdaptiveRangeAbsoluteValues(
                  residual, encodeFrequencyTableInBitStream,
                  maxLog2TrDynamicRange + 1);
        }

        // IF CURRENT DIRMODE IS BETTER
        if (currEntropy < entropy) {
          entropy = currEntropy;
          si.intraDir[compID] = dirMode;
        }
      }
      //            if(i < 3)
      //            {
      //                cout << "icu " << i << ", comID " << compID << ",
      //                refSampl " << refSamplToStr(refSampSide) << ", best
      //                intra mode " << UInt(si.intraDir[compID]) << ", entropy
      //                " << entropy << endl;
      //            }

      //            if(i==0)
      //                cout << "entropy= " << entropy << endl;
      intraPred.predIntraAng(roiComp_predictionFrameBuff, si.intraDir[compID]);
      for (UInt j = 0; j < n_tus; j++) {
        TransformUnit &curr_tu_si = si_TUs[j];
        transform2D.transformNxN(curr_tu_si.getTransformCoeffs(compID),
                                 predictionUnitBuf, curr_tu_si.block(compID),
                                 bitDepths[chType], maxLog2TrDynamicRange);
        quant.quant(curr_tu_si.getQuantizedTransformCoeffs(compID),
                    curr_tu_si.getTransformCoeffs(compID), uiAbsSum, cQP,
                    SliceType::I_SLICE, bitDepths[chType],
                    maxLog2TrDynamicRange);
      }

      //      if (oSiResult != nullptr) {
      //        cv::Mat predMat;
      //        VTM_OPENCV_Utility::toBGRMat(predictionUnitBuf, predMat);
      //        oSiResult->insert_or_assign(refSampSide, predMat);
      //      }
    }
  }
}

void icov::video::EncodingStructure::deallocate() {
  if (_requestedICUs.buf) {
    std::cout << "deallocating encoding structure" << std::endl;
    xFree(_requestedICUs.buf);
    _requestedICUs = AreaBuf<bool>();
    //        requestedBlocks.buf = nullptr;
    //        requestedBlocks.stride = 0;
  }

  if (_decodedICUs.buf) {
    xFree(_decodedICUs.buf);
    _decodedICUs = AreaBuf<bool>();
    //        previousDecodedBlocks.buf = nullptr;
    //        previousDecodedBlocks.stride = 0;
  }

  //    _tmp_alreadyProcessedICUs
  if (_tmp_alreadyProcessedICUs.buf) {
    xFree(_tmp_alreadyProcessedICUs.buf);
    _tmp_alreadyProcessedICUs = AreaBuf<bool>();
  }

  _vec_icus.clear();
  _vec_checkPoints.clear();
  _vec_decodedICUs.clear();
  _vec_pixelsRequestedPerICU.clear();
  _vec_tmp_alreadyProcessedICUs.clear();

  _n_icusPerRow = 0;
  _n_icusPerCol = 0;
}

icov::video::IcuEncoder &
icov::video::EncodingStructure::getICU(const UInt icu_index) {
#ifdef ICOV_ASSERTION_ENABLED
  ICOV_ASSERT_MSG(icu_index >= 0 && icu_index < _vec_icus.size(),
                  "Cannot retrieve ICU " + std::to_string(icu_index) +
                      " from vector of size " +
                      std::to_string(_vec_icus.size()));
#endif
  return _vec_icus[icu_index];
}

icov::video::IcuEncoder &
icov::video::EncodingStructure::getICU(const UInt x_index, const UInt y_index) {
  return getICU(y_index * _n_icusPerRow + x_index);
}

const icov::video::IcuEncoder &
icov::video::EncodingStructure::getICU(const UInt icu_index) const {
  ICOV_ASSERT_MSG(icu_index >= 0 && icu_index < _vec_icus.size(),
                  "Cannot retrieve ICU " + std::to_string(icu_index) +
                      " from vector of size " +
                      std::to_string(_vec_icus.size()));
  return _vec_icus[icu_index];
}

bool icov::video::EncodingStructure::isLeftVerticalBorder(
    const UInt icu_index) const {
  return (icu_index < _vec_icus.size()) && (icu_index % _n_icusPerRow == 0);
}

bool icov::video::EncodingStructure::isRightVerticalBorder(
    const UInt icu_index) const {
  return (icu_index < _vec_icus.size()) &&
         (icu_index % _n_icusPerRow == _n_icusPerRow - 1);
}

UInt icov::video::EncodingStructure::get_icuIndex(const Position &pos) const {
  return UInt(pos.y) * _n_icusPerRow + UInt(pos.x);
}

const std::vector<icov::video::IcuEncoder> &
icov::video::EncodingStructure::vec_icus() const {
  return _vec_icus;
}

UInt icov::video::EncodingStructure::n_icusPerRow() const {
  return _n_icusPerRow;
}

UInt icov::video::EncodingStructure::n_icusPerCol() const {
  return _n_icusPerCol;
}

size_t icov::video::EncodingStructure::n_icus() const {
  return _vec_icus.size();
}
