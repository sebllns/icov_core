#include "FrameEncoder.h"

#include "enc_helpers.h"
#include "helpers.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include "video_layer/ICOV_Quant.h"
#include "video_layer/ICOV_Transform2d.h"
#include "video_layer/vtm_opencv_utility.h"
#include "video_layer/vtm_utility.h"

#include <boost/algorithm/algorithm.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

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
      m_numComp(getNumberValidComponents(p.chFormat)) {
  std::unique_ptr<AffInteractiveCoder> ycoder, uvcoder;
  icov::CoderFactory::create_coder_yuv(p.program_args, ycoder, uvcoder);
  m_coders[ycoder->get_n()] = std::move(ycoder);
  m_coders[uvcoder->get_n()] = std::move(uvcoder);
}

/*****************************************************************************/
FrameEncoder::~FrameEncoder() { ICOV_LOG(trace) << "Destroy FrameEncoder"; }

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
      BlockPrediction::LEFT,         BlockPrediction::RIGHT,
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

  icov::video::IntraPrediction intraPred;
  intraPred.init(m_params.chFormat);

  ICOV_LOG(debug) << "Start computing intra predictions";
  for (UInt ch = 0; ch < m_numComp; ch++) {
    ICOV_LOG(debug) << "Channel " << ch + 1 << " / " << m_numComp;

    const ComponentID compID = ComponentID(ch);
    copyBordersFor360_1(mainFrameReco, compID);

    m_enc_struc.applyPredictionBasedOnEntropy(
        m_vvc_bufs, intraPred, compID, m_params.internalBitDepth,
        m_params.tuSizeAfterRegrouping_luma.width,
        m_params.tuSizeAfterRegrouping_luma.height, false, m_params.qp,
        m_params.maxLog2TrDynamicRange);
  }
  ICOV_LOG(debug) << "Successfully computed intra predictions";

  //   END INTRA PRED
}

/*****************************************************************************/
void icov::encoder::FrameEncoder::encode_tu(
    std::ostream &output, const Block &b,
    const icov::video::IcuEncoder &icu_enc,
    const icov::video::SideInformationUnit &si, BlockPrediction::predType pred,
    ComponentID comp, int tu_index) {

  boost::archive::binary_oarchive o_archive(output, boost::archive::no_header);

  const auto &x_tu = icu_enc.get_sourceX().TUs[tu_index];
  auto x_tuCoeff = x_tu.getQuantizedTransformCoeffs(comp);

  size_t tu_size = x_tuCoeff.area();

  const auto &si_tu = si.TUs[tu_index];
  auto si_tuCoeff = si_tu.getQuantizedTransformCoeffs(comp);

  ICOV_ASSERT_MSG(
      (size_t)si_tuCoeff.area() == tu_size,
      "Transform units from Source and its SI must be the same size: " +
          std::to_string(tu_size));

  ICOV_ASSERT_MSG(m_coders.count(tu_size) > 0,
                  "Coder not found for the given size: " +
                      std::to_string(tu_size));

  BitSymbolProvider bsp(SymboleVec(x_tuCoeff.buf, x_tuCoeff.buf + tu_size));
  bsp.set_si(SIVec(si_tuCoeff.buf, si_tuCoeff.buf + tu_size),
             m_params.bsp_params);

  icov::SINumcodeIdx si_numcodes;
  icov::SISyndromes si_synd;

  m_coders.at(tu_size)->encode_bitplanes(bsp, si_numcodes, si_synd);

  BitSymbolProvider bsp_dec(tu_size);
  bsp_dec.set_si(bsp.si_symboles(), bsp.z_min(), bsp.z_max(), bsp.q0(),
                 bsp.x_binary_length());

  for (size_t i = 0; i < si_numcodes.size(); i++) {
    output << b.getId(Block::SELF) << "\t" << b.parent()->row(b.v()) << "\t"
           << b.parent()->col(b.u()) << "\t" << BlockPrediction::toString(pred)
           << "\t" << tu_index << "\t" << (int)comp << "\t" << i << "\t"
           << m_coders.at(tu_size)->get_r(si_numcodes[i]) << "\t"
           << si_synd[i].to_string(" ") << "\t";
    output << std::endl;

    if (m_params.check_decoding) {

      ICOV_LOG(debug) << "Check decoding bitplane " << i << std::endl;

      BitVec decoded;

      try {
        ICOV_LOG(debug) << "Check bitplane " << i;
        m_coders.at(tu_size)->decode_bitplane(decoded, i, bsp_dec,
                                              si_numcodes[i], si_synd[i]);
        icov::helpers::assert::check(decoded == bsp.src_bits()[i],
                                     "Decoding BP check failed " +
                                         std::to_string(i));
      } catch (const std::exception &e) {
        ICOV_LOG(error) << e.what();
        break;
      }
    }
  }

  if (m_params.check_decoding) {
    try {
      ICOV_LOG(debug) << "Decode channel " << comp;

      bsp_dec.set_si(bsp.si_symboles(), bsp.z_min(), bsp.z_max(), bsp.q0(),
                     bsp.x_binary_length());
      m_coders.at(tu_size)->decode_symboles(bsp_dec, si_numcodes, si_synd);

      icov::helpers::assert::check(bsp_dec.src_symboles() == bsp.src_symboles(),
                                   "Decoding symbole failed for channel " +
                                       std::to_string(comp));

      ICOV_LOG(info) << "Channel was successfully decoded";
    } catch (const std::exception &e) {
      ICOV_LOG(error) << e.what();
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
  ICOV_LOG(info) << "Block was successfully encoded";

  if (m_params.check_decoding) {
    ICOV_LOG(info) << "Decoding check passed";
  }
}

void icov::encoder::FrameEncoder::encode_icu(
    std::ostream &output, const Block &b,
    const icov::video::IcuEncoder &icu_enc, BlockPrediction::predType pred) {

  ICOV_LOG(info) << "Encode ICU with prediction "
                 << BlockPrediction::toString(pred) << std::endl;

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

  ICOV_LOG(debug) << "try encode prediction " << BlockPrediction::toString(pred)
                  << std::endl;

  int x_block = b.parent()->col(b.u());
  int y_block = b.parent()->row(b.v());

  Position pos(static_cast<float>(x_block) /
                   static_cast<float>(m_params.icuSize_luma.width),
               static_cast<float>(y_block) /
                   static_cast<float>(m_params.icuSize_luma.height));

  ICOV_LOG(trace) << pos.x;
  ICOV_LOG(trace) << pos.y;

  const auto &es = this->m_enc_struc;
  uint icuIndex = es.get_icuIndex(pos);

  ICOV_LOG(trace) << icuIndex;

  const auto &icu_enc = es.getICU(icuIndex);
  const auto &si_map = icu_enc.get_intraSIsMap();

  if (si_map.count(pred) > 0)
    encode_icu(output, b, icu_enc, pred);
}
