#include "BlockDecoder.h"

#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include "video_layer/ICOV_Quant.h"
#include "video_layer/ICOV_Transform2d.h"
#include "video_layer/vtm_opencv_utility.h"
#include "video_layer/vtm_utility.h"

#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

#define WIN_ORIG "Original"
#define WIN_RECO "Reconstruction"
#define WIN_PRED "Predictions"
#define WIN_DEBUG_DECODE "Decoded"

namespace fs = boost::filesystem;
using namespace icov::decoder;
using SI_SIDE = BlockPrediction::predType;

BlockDecoder::BlockDecoder(const params &p) : IBlockDecoder(), m_params(p) {
  // CONSTANTS
  const Size icuSize_luma =
      Size(m_params.icuSize_luma.width, m_params.icuSize_luma.height);
  const Size tuSize_luma =
      Size(m_params.tuSize_luma.width, m_params.tuSize_luma.height);

  m_dec_struc
      .create360VideoInteractiveCodingUnits_withoutAccessBlockComputation(
          m_params.chFormat, m_params.sourceSize.width,
          m_params.sourceSize.height, m_params.margin_x_360luma,
          icuSize_luma.width, icuSize_luma.height, tuSize_luma.width,
          tuSize_luma.height);
}

BlockDecoder::~BlockDecoder() {}

void icov::decoder::BlockDecoder::startProcess() {}

void icov::decoder::BlockDecoder::processBlock(const Block &b,
                                               uint16_t prediction) {

  BlockPrediction::predType pred = BlockPrediction::toPredType(prediction);

  std::cout << "---- try encode prediction " << BlockPrediction::toString(pred)
            << std::endl;

  int x_block = b.parent()->col(b.u());
  int y_block = b.parent()->row(b.v());

  Position pos(static_cast<float>(x_block) /
                   static_cast<float>(m_params.icuSize_luma.width),
               static_cast<float>(y_block) /
                   static_cast<float>(m_params.icuSize_luma.height));

  std::cout << pos.x << std::endl;
  std::cout << pos.y << std::endl;

  const auto &es = this->m_dec_struc;
  uint icuIndex = es.get_icuIndex(pos);

  std::cout << icuIndex << std::endl;

  const auto &icu_enc = es.getICU(icuIndex);
  const auto &si_map = icu_enc.get_intraSIsMap();

  if (si_map.count(pred) > 0)
    decode_icu(b, icu_enc, pred);

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

void BlockDecoder::decode_icu(const Block &b, const video::IcuEncoder &icu_enc,
                              BlockPrediction::predType pred) {}
