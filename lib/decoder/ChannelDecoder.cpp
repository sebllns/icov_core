#include "ChannelDecoder.h"

#include "Common.h"
#include "Unit.h"
#include "common_types.h"
#include "helpers.h"
#include "icov_stream.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include "typedef.h"
#include "video_layer/ICOV_Quant.h"
#include "video_layer/ICOV_Transform2d.h"
#include "video_layer/vtm_opencv_utility.h"
#include "video_layer/vtm_utility.h"

#include <algorithm>
#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

namespace fs = boost::filesystem;
using namespace icov::decoder;
using SI_SIDE = BlockPrediction::predType;

ChannelDecoder::ChannelDecoder(
    const params &p, const LdpcCoderParameters &LDPC_params,
    const std::shared_ptr<icov::video::BufGroups> &vvc_bufs,
    const std::shared_ptr<video::EncodingStructure> &encodingStruct,
    const sp_IcovExtractedBitstream &extraction)
    : m_params(p), m_vvc_bufs(vvc_bufs), m_encodingStruct(encodingStruct),
      //      m_blockArea(
      //          UnitArea(p.QpParam.chromaFomat, Area(0, 0, p.icu_size,
      //          p.icu_size))
      //              .block(p.CompID)),
      m_ldpc_coder(icov::LdpcCoderFactory::create_coder(LDPC_params)),
      m_blockBitplane(m_ldpc_coder->get_n(), 0),
      m_blockSI(m_ldpc_coder->get_n(), 0),
      m_syndrome(m_ldpc_coder->get_m_max()), m_bsp(m_ldpc_coder->get_n()),
      m_block_prediction(BlockPrediction::predType::UNKNOWN),
      m_bitstream(extraction) {
  intraPred.init(m_params.QpParam.chromaFomat);
}

ChannelDecoder::~ChannelDecoder() {}

void ChannelDecoder::decode_icu(int icu_enc,
                                BlockPrediction::predType block_prediction) {
  ICOV_ASSERT_MSG(block_prediction != BlockPrediction::predType::UNKNOWN,
                  "Prediction unknown;")

  m_icuIndex = icu_enc;

  m_block_prediction = block_prediction;
  std::fill(m_blockSI.begin(), m_blockSI.end(), 0);
  std::fill(m_blockBitplane.begin(), m_blockBitplane.end(), 0);

  //  const auto comp_area_value = m_blockArea.area();

  std::cout << "COMP " << m_params.CompID;

  if (block_prediction != BlockPrediction::predType::NONE) {
    std::cout << "DECODE BLOCK WITH PRED "
              << BlockPrediction::toString(block_prediction) << std::endl;
  } else {
    std::cout << "DECODE ACCESS BLOCK\n";
  }

  int bitplaneCount = (int)m_bitstream->pop();
  std::cout << " BT " << bitplaneCount << std::endl;

  for (int i = 0; i < bitplaneCount; ++i) {
    icov::byte quantizedNumcode =
        (block_prediction == BlockPrediction::predType::NONE)
            ? 1
            : m_bitstream->pop();

    int syndrome_size =
        bitstream::inverseQuantizedSize(quantizedNumcode, *m_ldpc_coder);
    read_syndrome(syndrome_size);

    decode_bitplane(i, static_cast<icov::NumcodeIdx>(quantizedNumcode),
                    m_syndrome);
  }

  end_decode_icu();
}

void ChannelDecoder::set_si_dir_mode(int dir_mode) {

  auto mp_icu_enc = m_encodingStruct->getICU(m_icuIndex);
  auto m_blockArea = mp_icu_enc.get_sourceX().block(m_params.CompID);

  auto roiComp_predictionFrameBuff =
      VTM_Utility::getAreaBuf(m_vvc_bufs->get(PIC_PREDICTION), m_blockArea);

  intraPred.initIntraPatternChType(m_vvc_bufs->getBuf(PIC_RECONSTRUCTION),
                                   m_block_prediction, m_blockArea,
                                   m_params.bit_depth, false);

  auto currICUintraMap = mp_icu_enc.get_intraSIsMap();
  auto &si = currICUintraMap.at(m_block_prediction);

  si.intraDir[m_params.CompID] = dir_mode;
  intraPred.predIntraAng(roiComp_predictionFrameBuff, dir_mode);

  ICOV_ASSERT_MSG(si.TUs.size() == 1,
                  "This version of ICOV only supports one TU per ICU");

  auto &curr_tu_si = si.TUs[0];

  const auto &predictionUnitBuf = m_vvc_bufs->get(PIC_PREDICTION);

  transform2D.transformNxN(curr_tu_si.getTransformCoeffs(m_params.CompID),
                           predictionUnitBuf, curr_tu_si.block(m_params.CompID),
                           m_params.bit_depth, m_params.maxLog2TrDynamicRange);
  quant.quant(curr_tu_si.getQuantizedTransformCoeffs(m_params.CompID),
              curr_tu_si.getTransformCoeffs(m_params.CompID), uiAbsSum,
              m_params.QpParam, SliceType::I_SLICE, m_params.bit_depth,
              m_params.maxLog2TrDynamicRange);
}

void ChannelDecoder::set_access_block_params(int lx) {
  m_bsp.set_si(m_blockSI, 0, 0, 0, lx);
}

void ChannelDecoder::set_si_params(int zmin, int zmax, float q, int lx) {
  auto mp_icu_enc = m_encodingStruct->getICU(m_icuIndex);
  auto currICUintraMap = mp_icu_enc.get_intraSIsMap();
  const auto &si = currICUintraMap.at(m_block_prediction);

  const auto &si_tu = si.TUs[0];
  auto si_tuCoeff = si_tu.getQuantizedTransformCoeffs(m_params.CompID);

  // SI is perfect => clone it
  if (lx == 0) {
    icov::video::TransformUnit &dec_tu = mp_icu_enc.sourceX_TU(0);
    std::copy(si_tuCoeff.buf, si_tuCoeff.buf + m_ldpc_coder->get_n(),
              dec_tu.getQuantizedTransformCoeffs(m_params.CompID).buf);
  } else {
    m_blockSI.assign(si_tuCoeff.buf, si_tuCoeff.buf + m_ldpc_coder->get_n());
    m_bsp.set_si(m_blockSI, zmin, zmax, q, lx);
  }
}

void ChannelDecoder::read_syndrome(int syndrome_size) {
  m_syndrome.resize(syndrome_size);
  icov::byte bitstream_value = 0;
  for (int i = 0; i < syndrome_size; ++i) {
    if ((i % BYTE_SIZE) == 0) {
      bitstream_value = m_bitstream->pop();
    }
    m_syndrome[i] = ((bitstream_value & (i % BYTE_SIZE)) > 0);
  }
}

void ChannelDecoder::decode_bitplane(int bp, NumcodeIdx numcode,
                                     const Syndrome &synd) {
  if (m_block_prediction == BlockPrediction::predType::NONE) {
    ICOV_ASSERT_MSG(numcode == 1,
                    "Numcode value must be set to 1 for access block");
  }
  m_ldpc_coder->decode_bitplane(m_blockBitplane, bp, m_bsp, numcode, synd);

  auto mp_icu_enc = m_encodingStruct->getICU(m_icuIndex);

  if ((bp + 1) == m_bsp.x_binary_length()) {
    icov::video::TransformUnit &dec_tu = mp_icu_enc.sourceX_TU(0);
    std::copy(m_bsp.src_symboles().begin(), m_bsp.src_symboles().end(),
              dec_tu.getQuantizedTransformCoeffs(m_params.CompID).buf);
    icov::helpers::io::write_vector(std::cout, m_bsp.src_symboles(), ' ');
    std::cout << std::endl;
  }
}

void ChannelDecoder::end_decode_icu() {

  auto mp_icu_enc = m_encodingStruct->getICU(m_icuIndex);

  icov::video::TransformUnit &dec_tu = mp_icu_enc.sourceX_TU(0);

  // UnitArea blockArea;
  // PelStorage dec_block;
  //  dec_block.create(blockArea);

  // iQ
  quant.dequant(dec_tu.getInverseQuantizedTransformCoeffs(m_params.CompID),
                dec_tu.getQuantizedTransformCoeffs(m_params.CompID),
                m_params.QpParam, m_params.bit_depth,
                m_params.maxLog2TrDynamicRange);

  // iDCT
  transform2D.invTransformNxN(
      m_vvc_bufs->get(PIC_RECONSTRUCTION), dec_tu.block(m_params.CompID),
      dec_tu.getInverseQuantizedTransformCoeffs(m_params.CompID),
      m_params.bit_depth, m_params.maxLog2TrDynamicRange);

  m_block_prediction = BlockPrediction::predType::UNKNOWN;
}

Syndrome &ChannelDecoder::syndrome() { return m_syndrome; }

const AffInteractiveCoder &ChannelDecoder::ldpc_coder() const {
  return *m_ldpc_coder;
}
