#include "FrameEncoder.h"

#include "BitSymbolProvider.h"
#include "enc_helpers.h"
#include "helpers.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include "tools/DebugTools.h"
#include "video_layer/vtm_opencv_utility.h"
#include "video_layer/vtm_utility.h"

#include <boost/algorithm/algorithm.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/serialization/vector.hpp>

#include <cstdint>
#include <omp.h>

namespace fs = boost::filesystem;
using namespace icov::encoder;
using SI_SIDE = BlockPrediction::predType;

/*****************************************************************************/
void copyBordersFor360_1(PelUnitBuf &mainFrame_roi, const ComponentID compID) {
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

/*****************************************************************************/
FrameEncoder::FrameEncoder(int frame, const params &p)
    : IFrameEncoder(frame), m_params(p),
      m_numComp(getNumberValidComponents(p.chFormat)),
      m_pred_encoder(p.program_args, std::vector<uint16_t>{
                                         BlockPrediction::RIGHT,
                                         BlockPrediction::LEFT,
                                         BlockPrediction::TOP,
                                         BlockPrediction::BOTTOM,
                                         BlockPrediction::TOP_RIGHT,
                                         BlockPrediction::TOP_LEFT,
                                         BlockPrediction::BOTTOM_RIGHT,
                                         BlockPrediction::BOTTOM_LEFT,
                                     }) {

  int idx = 0;
  for (auto pred : BlockPrediction::getAvailablePredictions())
    m_predIndexes[static_cast<uint16_t>(pred)] = idx++;
}

/*****************************************************************************/
FrameEncoder::~FrameEncoder() { ICOV_TRACE << "Destroy FrameEncoder"; }

/*****************************************************************************/
void FrameEncoder::encodeBlock(std::ostream &output, const Block &b,
                               bool access_block,
                               const IBlockPrediction &block_prediction) {

  ICOV_DEBUG << "start encode block";
  const auto v = getAvailablePredictions(b, access_block, block_prediction);
  m_stored_synd.clear();
  m_stored_numcodes = SINumcodeIdxVec(v.size());

#pragma omp parallel for
  for (uint16_t p : v)
    encodeBlockWithPrediction(output, b, p);

  boost::archive::binary_oarchive o_archive(output, boost::archive::no_header);
  //  //save rates for each
  //  for (size_t i = 0; i < si_numcodes.size(); i++) {
  //    output << b.getId(Block::SELF) << "\t" << b.parent()->row(b.v()) << "\t"
  //           << b.parent()->col(b.u()) << "\t" <<
  //           BlockPrediction::toString(pred)
  //           << "\t" << tu_index << "\t" << (int)comp << "\t" << i << "\t"
  //           << encoder.get_r(si_numcodes[i]) << "\t" <<
  //           si_synd[i].to_string(" ")
  //           << "\t" << std::endl;
  //  }

  ICOV_DEBUG << "end encode block";
}

/*****************************************************************************/
void FrameEncoder::processInput(std::ostream &output,
                                const IInputMedia<cv::Mat> &input) {

  cv::Mat img_src = input.getCurrentFrame();

  ICOV_ASSERT_MSG(img_src.total() > 0, "Cannot encode empty image");

  // CONSTANTS
  const Size icuSize_luma =
      Size(m_params.icuSize_luma.width, m_params.icuSize_luma.height);
  const Size tuSize_luma =
      Size(m_params.tuSize_luma.width, m_params.tuSize_luma.height);

  // end constants

  // resize
  cv::Size size_before = img_src.size();
  resize_covering_blocks(img_src,
                         cv::Size(icuSize_luma.width, icuSize_luma.height));
  cv::Size size_after = img_src.size();

  if (size_after != size_before) {
    ICOV_LOG(warning) << "Image resized from " << size_before << " to "
                      << size_after;

    ICOV_LOG(warning) << "Aspect changed from "
                      << double(size_before.width) / double(size_before.height)
                      << " to "
                      << double(size_after.width) / double(size_after.height);
  }

  const int sourceWidth = img_src.cols;
  const int sourceHeight = img_src.rows;
  // end resize

  UnitArea extendedFrameArea(
      m_params.chFormat,
      Area(0, 0, sourceWidth + 2 * m_params.margin_x_360luma, sourceHeight));
  UnitArea mainFrameArea(m_params.chFormat, Area(m_params.margin_x_360luma, 0,
                                                 sourceWidth, sourceHeight));

  //  PelStorage picTemp;
  //  picTemp.create(extendedFrameArea);

  m_vvc_bufs.create(PIC_ORIGINAL, extendedFrameArea);
  m_vvc_bufs.create(PIC_RECONSTRUCTION, extendedFrameArea);
  m_vvc_bufs.create(PIC_PREDICTION, extendedFrameArea);

  std::vector<BlockPrediction::predType> predVec = {
      BlockPrediction::RIGHT,        BlockPrediction::LEFT,
      BlockPrediction::TOP,          BlockPrediction::BOTTOM,
      BlockPrediction::TOP_LEFT,     // 0b00101
      BlockPrediction::BOTTOM_LEFT,  // 0b01001
      BlockPrediction::BOTTOM_RIGHT, // 0b01010
      BlockPrediction::TOP_RIGHT};

  for (auto pk : predVec) {
    m_vvc_bufs.create_link(PIC_PREDICTION + pk, PIC_PREDICTION);
  }

  PelUnitBuf mainFrameOrig = m_vvc_bufs.get(PIC_ORIGINAL).getBuf(mainFrameArea);

  ICOV_ASSERT_MSG((int)(mainFrameOrig.get(COMPONENT_Y).width) == img_src.cols,
                  "Width does not match");

  ICOV_ASSERT_MSG((int)(mainFrameOrig.get(COMPONENT_Y).height) == img_src.rows,
                  "Height does not match");

  VTM_OPENCV_Utility::cvMatToYUV(img_src, mainFrameOrig);
  m_enc_struc
      .create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
          m_params.chFormat, sourceWidth, sourceHeight,
          m_params.margin_x_360luma, icuSize_luma.width, icuSize_luma.height,
          tuSize_luma.width, tuSize_luma.height);

  VTM_Utility::quantizeFrame(
      m_params.qp, m_vvc_bufs.get(PIC_ORIGINAL),
      m_vvc_bufs.get(PIC_RECONSTRUCTION), m_enc_struc,
      m_params.internalBitDepth,
      m_params.maxLog2TrDynamicRange); // give the whole extended image (
                                       // .getOrigBuf()
  // and
  // .getRecoBuf() ) to the function because vector
  // icues_enc is based on extended frames

  // INTRA PRED
  PelUnitBuf mainFrameReco =
      m_vvc_bufs.get(PIC_RECONSTRUCTION).getBuf(mainFrameArea);

  ICOV_DEBUG << "Start computing intra predictions";
  for (UInt ch = 0; ch < m_numComp; ch++) {
    ICOV_DEBUG << "Channel " << ch + 1 << " / " << m_numComp;

    const ComponentID compID = ComponentID(ch);
    copyBordersFor360_1(mainFrameReco, compID);

    m_enc_struc.applyPredictionBasedOnEntropy(
        m_vvc_bufs, compID, m_params.internalBitDepth,
        m_params.tuSizeAfterRegrouping_luma.width,
        m_params.tuSizeAfterRegrouping_luma.height, false, m_params.qp,
        m_params.maxLog2TrDynamicRange);
  }
  ICOV_DEBUG << "Successfully computed intra predictions";

  //   END INTRA PRED
}

/*****************************************************************************/
void icov::encoder::FrameEncoder::encode_tu(
    std::ostream &output, const Block &b,
    const icov::video::IcuEncoder &icu_enc,
    const icov::video::SideInformationUnit &si, BlockPrediction::predType pred,
    ComponentID comp, int tu_index) {

  const auto predID = static_cast<uint16_t>(pred);

  const auto &x_tu = icu_enc.get_sourceX().TUs[tu_index];
  const auto &si_tu = si.TUs[tu_index];

  auto x_tuCoeff = x_tu.getQuantizedTransformCoeffs(comp);

  size_t tu_size = x_tuCoeff.area();

  // encode outputs for the current SI: vector of rates and vector of syndrome
  // (one rate and syndrome per bitplane)
  icov::NumcodeIdxVec si_numcodes;
  icov::SyndromesVec si_synd;

  BitSymbolProvider bsp =
      x_tu.getBitSymbolProvider(comp, si_tu, m_params.bsp_params);

  auto &encoder = m_pred_encoder.get_coder(predID, tu_size);
  encoder.encode_bitplanes(bsp, si_numcodes, si_synd);

  // CHECK DECODING
  if (m_params.check_decoding) {

    BitSymbolProvider bsp_dec(tu_size);

    bsp_dec.set_si(bsp.si_symboles(), bsp.z_min(), bsp.z_max(), bsp.q0(),
                   bsp.x_binary_length());

    for (size_t i = 0; i < si_numcodes.size(); i++) {

      ICOV_TRACE << "Check decoding bitplane " << i << std::endl;
      BitVec decoded;
      try {
        ICOV_TRACE << "Check bitplane " << i;
        encoder.decode_bitplane(decoded, i, bsp_dec, si_numcodes[i],
                                si_synd[i]);
        icov::helpers::assert::check(decoded == bsp.src_bits()[i],
                                     "Decoding BP check failed " +
                                         std::to_string(i));
      } catch (const std::exception &e) {
        ICOV_ERROR << e.what();
        break;
      }
    }

    try {
      bsp_dec.set_si(bsp.si_symboles(), bsp.z_min(), bsp.z_max(), bsp.q0(),
                     bsp.x_binary_length());
      encoder.decode_symboles(bsp_dec, si_numcodes, si_synd);

      icov::helpers::assert::check(bsp_dec.src_symboles() == bsp.src_symboles(),
                                   "Decoding symbole failed for channel " +
                                       std::to_string(comp));
    } catch (const std::exception &e) {
      ICOV_ERROR << e.what();
    }
  }
// END CHECK DECODING

// encoding and decoding check done, now save syndrome if longer than current
// saved
#pragma omp critical
  {
    ICOV_DEBUG << "id " << predID;
    ICOV_DEBUG << "value " << m_predIndexes[predID];
    ICOV_DEBUG << "size  " << m_stored_numcodes.size();

    m_stored_numcodes[m_predIndexes[predID]] = si_numcodes;
    if (m_stored_synd.size() == 0) {
      m_stored_synd = si_synd;
    } else {
      icov::helpers::assert::equal(
          m_stored_synd.size(), si_synd.size(),
          "Bitplane count does not match between SI of same block");
      for (size_t i = 0; i < m_stored_synd.size(); ++i) {
        if (si_synd[i].size() > m_stored_synd[i].size()) {
          m_stored_synd[i] = si_synd[i];
        }
      }
    }
  }
}

void icov::encoder::FrameEncoder::encode_si(
    std::ostream &output, const Block &b,
    const icov::video::IcuEncoder &icu_enc,
    const icov::video::SideInformationUnit &si, BlockPrediction::predType pred,
    ComponentID comp) {
  for (size_t tu_index = 0; tu_index < si.TUs.size(); tu_index++) {
    encode_tu(output, b, icu_enc, si, pred, comp, tu_index);
  }
}

void icov::encoder::FrameEncoder::encode_icu(
    std::ostream &output, const Block &b,
    const icov::video::IcuEncoder &icu_enc, BlockPrediction::predType pred) {

  auto si = icu_enc.get_sideInformation(pred);
  icov::helpers::assert::not_null(si);
  for (uint c = 0; c < m_numComp; c++) {
    encode_si(output, b, icu_enc, *si, pred, static_cast<ComponentID>(c));
  }
}

/*****************************************************************************/
void FrameEncoder::encodeBlockWithPrediction(std::ostream &output,
                                             const Block &b,
                                             uint16_t prediction) {

  BlockPrediction::predType pred = BlockPrediction::toPredType(prediction);

  int x_block = b.parent()->col(b.u());
  int y_block = b.parent()->row(b.v());

  Position pos(static_cast<float>(x_block) /
                   static_cast<float>(m_params.icuSize_luma.width),
               static_cast<float>(y_block) /
                   static_cast<float>(m_params.icuSize_luma.height));

  const auto &es = this->m_enc_struc;
  uint icuIndex = es.get_icuIndex(pos);

  const auto &icu_enc = es.getICU(icuIndex);
  const auto &si_map = icu_enc.get_intraSIsMap();

  if (si_map.count(pred) > 0)
    encode_icu(output, b, icu_enc, pred);
}
