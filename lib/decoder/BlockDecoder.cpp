#include "BlockDecoder.h"
#include "decoder/ChannelDecoder.h"

#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "video_layer/Encodingstructure.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/ICOV_Quant.h"
#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

namespace fs = boost::filesystem;
using namespace icov::decoder;
using SI_SIDE = icov::omnidirectional_layer::BlockPrediction::ePredType;

BlockDecoder::BlockDecoder(
    const params &p, const LdpcCoderParameters &coder_params,
    const bitstream::sp_IcovExtractedBitstream &extraction)
    : IBlockDecoder(), m_params(p) {
  // CONSTANTS
  const Size icuSize_luma =
      Size(m_params.icuSize_luma.width, m_params.icuSize_luma.height);
  const Size tuSize_luma =
      Size(m_params.icuSize_luma.width, m_params.icuSize_luma.height);

  const int sourceWidth = (int)p.sourceSize.width;
  const int sourceHeight = (int)p.sourceSize.height;
  const int margin_x_360luma = 2;

  const UnitArea extendedFrameArea(
      p.chFormat,
      Area(0, 0, sourceWidth + 2 * margin_x_360luma, sourceHeight)); //???
  mainFrameArea = UnitArea(
      p.chFormat, Area(margin_x_360luma, 0, sourceWidth, sourceHeight));

  m_vvc_bufs = std::make_shared<icov::video::BufGroups>();
  m_vvc_bufs->create(PIC_ORIGINAL, extendedFrameArea);
  m_vvc_bufs->create(PIC_RECONSTRUCTION, extendedFrameArea);
  m_vvc_bufs->create(PIC_PREDICTION, extendedFrameArea);

  m_dec_struc = std::make_shared<video::EncodingStructure>();

  m_dec_struc
      ->create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
          m_params.chFormat, m_params.sourceSize.width,
          m_params.sourceSize.height, m_params.margin_x_360luma,
          icuSize_luma.width, icuSize_luma.height, tuSize_luma.width,
          tuSize_luma.height);

  const int channelCount = getNumberValidComponents(p.chFormat);

  UnitArea blockArea(p.chFormat,
                     Area(0, 0, p.icuSize_luma.width, p.icuSize_luma.height));

  for (int c = 0; c < channelCount; c++) {
    const auto compID = static_cast<ComponentID>(c);
    auto coder_channel_params = coder_params.clone();
    coder_channel_params->codeLength = blockArea.block(compID).area();

    const ChannelType channelType = toChannelType(compID);
    const int bit_depth = p.internalBitDepth[channelType];
    const icov::video::QpParam cQP(p.qp, channelType, p.chFormat, bit_depth, 0,
                                   0);
    ChannelDecoder::params channel_params{

        .QpParam = cQP,

        .CompID = compID,

        .bit_depth = bit_depth,

        .maxLog2TrDynamicRange = p.maxLog2TrDynamicRange

    };

    m_channelDecoder.push_back(
        std::make_unique<ChannelDecoder>(channel_params, *coder_channel_params,
                                         m_vvc_bufs, m_dec_struc, extraction));
  }
}

BlockDecoder::~BlockDecoder() {}

void icov::decoder::BlockDecoder::startProcess() {}

void icov::decoder::BlockDecoder::processBlock(
    const omnidirectional_layer::Block &b,
    omnidirectional_layer::IBlockPredictionManager::prediction_t prediction) {

  omnidirectional_layer::BlockPrediction::ePredType pred =
      omnidirectional_layer::BlockPrediction::toPredType(prediction);

  std::cout << "---- try decode prediction "
            << omnidirectional_layer::BlockPrediction::toString(pred)
            << std::endl;

  icov::assert::check(
      omnidirectional_layer::BlockPrediction::isValidPrediction(pred),
      "Bad prediction value");

  int x_block = b.parent()->col(b.u());
  int y_block = b.parent()->row(b.v());

  Position pos(static_cast<float>(x_block) /
                   static_cast<float>(m_params.icuSize_luma.width),
               static_cast<float>(y_block) /
                   static_cast<float>(m_params.icuSize_luma.height));

  std::cout << pos.x << std::endl;
  std::cout << pos.y << std::endl;

  const auto &es = *this->m_dec_struc;
  uint icuIndex = es.get_icuIndex(pos);

  std::cout << icuIndex << std::endl;

  const auto &icu_enc = es.getICU(icuIndex);
  //  const auto &si_map = icu_enc.get_intraSIsMap();

  //  if (si_map.count(pred) > 0)
  // decode_icu(b, icu_enc, pred);

  bitstream::VideoStreamDebugger::get_mutable_instance().reset();

  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingBlockID(
      b.getIdSelf() - 1);
  bitstream::VideoStreamDebugger::get_mutable_instance()
      .setDebuggingPredictionID(pred);

  for (auto &dec : m_channelDecoder) {
    ICOV_TRACE << "DECODE COMP " << dec->get_params().CompID;
    dec->decode_icu(icuIndex, pred);
    ICOV_TRACE << "END DECODE COMP " << dec->get_params().CompID;
  }

  //  // decoding test
  //  icov::SymboleVec decodedVec = bsp.src_symboles();
  //  int x_block = b.parent()->col(b.u());
  //  int y_block = b.parent()->row(b.v());

  //  Position pos(static_cast<float>(x_block) /
  //                   static_cast<float>(m_params.icuSize_luma.width),
  //               static_cast<float>(y_block) /
  //                   static_cast<float>(m_params.icuSize_luma.height));

  //  uint icuIndex = m_dec_struc.get_icuIndex(pos);
  //  auto &icu_dec = m_dec_struc.getICU(icuIndex);
  //  auto &tu_dec = icu_dec.sourceX_TU(tu_index);
  //  auto x_tuCoeff_dec = tu_dec.getQuantizedTransformCoeffs(comp);
  //  icov::video::Quant quant;
  //  icov::video::Transform2D transform2D;

  //  ChannelType channelType = toChannelType(comp);
  //  int bit_depth = m_params.internalBitDepth[channelType];

  //  icov::video::QpParam cQP(m_params.qp, channelType, m_params.chFormat,
  //                           bit_depth, 0, 0);

  //  // iQ
  //  quant.dequant(tu_dec.getInverseQuantizedTransformCoeffs(comp),
  //                tu_dec.getQuantizedTransformCoeffs(comp), cQP, bit_depth,
  //                m_params.maxLog2TrDynamicRange);

  //  // iDCT
  //  transform2D.invTransformNxN(quantizedImage, tu_dec.block(comp),
  //                              tu_dec.getInverseQuantizedTransformCoeffs(comp),
  //                              bit_depth, m_params.maxLog2TrDynamicRange);

  //  // m_enc_struc
  //  // .create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
  //  //                       m_params.chFormat, sourceWidth, sourceHeight,
  //  //                       margin_x_360luma, icuSize_luma.width,
  //  //                       icuSize_luma.height, tuSize_luma.width,
  //  //                       tuSize_luma.height);

  //  // TransformUnit decoded_tu();

  //  // m_coders.at(tu_size)->decode_bitplanes(decoded, si_numcodes, si_synd);
  //  // std::cout << (decoded == bsp.src_symboles()) << std::endl;
}

void icov::decoder::BlockDecoder::endProcess() {}

void icov::decoder::BlockDecoder::decode() {}

const PelUnitBuf BlockDecoder::decodingOutput() const {
  return m_vvc_bufs->get(PIC_RECONSTRUCTION).getBuf(mainFrameArea);
}

void BlockDecoder::decode_icu(
    const omnidirectional_layer::Block &b, const video::IcuEncoder &icu_enc,
    omnidirectional_layer::BlockPrediction::ePredType pred) {
  if (pred == omnidirectional_layer::BlockPrediction::NONE) {
    // access block
    std::cout << "Decode ACCESS BLOCK\n";
  } else {
  }
}
