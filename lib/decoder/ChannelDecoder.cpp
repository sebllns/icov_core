#include "ChannelDecoder.h"
#include "FastArray.h"
#include "Unit.h"

#include "icov_log.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/ICOV_Quant.h"
#include "video_layer/ICOV_Transform2d.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"
#include "video_layer/vtm_utility.h"
#include <algorithm>
#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <iostream>

namespace icov {
namespace decoder {

namespace fs = boost::filesystem;
using namespace icov::decoder;
using SI_SIDE = omnidirectional_layer::BlockPrediction::ePredType;

ChannelDecoder::ChannelDecoder(const params &p, const LdpcCoderParameters &LDPC_params,
                               const std::shared_ptr<icov::video::BufGroups> &vvc_bufs,
                               const std::shared_ptr<video::EncodingStructure> &encodingStruct,
                               const bitstream::sp_IcovExtractedBitstream &extraction)
    : m_params(p), m_vvc_bufs(vvc_bufs), m_encodingStruct(encodingStruct),
      //      m_blockArea(
      //          UnitArea(p.QpParam.chromaFomat, Area(0, 0, p.icu_size,
      //          p.icu_size))
      //              .block(p.CompID)),
      m_ldpc_coder(ldpc_codec_layer::LdpcCoderFactory::create_coder(LDPC_params)),
      m_blockBitplane(m_ldpc_coder->get_n(), 0), m_blockSI(m_ldpc_coder->get_n(), 0),
      m_syndrome(m_ldpc_coder->get_m_max()), m_bsp(m_ldpc_coder->get_n()),
      m_block_prediction(omnidirectional_layer::BlockPrediction::ePredType::UNKNOWN), m_bitstream(extraction),
      m_blockSymbols(m_ldpc_coder->get_n(), 0), m_rate(0)
{
    m_intraPred.init(m_params.QpParam.chromaFomat);
}

ChannelDecoder::~ChannelDecoder() {}

void icov::decoder::ChannelDecoder::_decode_bitplane(
    int i, omnidirectional_layer::BlockPrediction::ePredType block_prediction) {

  byte quantizedNumcode = m_numcodes.front();
  m_numcodes.pop();

  int syndrome_size = bitstream::inverseQuantizedSize(quantizedNumcode, *m_ldpc_coder);

  read_syndrome(syndrome_size);
  decode_bitplane_with_ldpc(i, static_cast<icov::NumcodeIdx>(quantizedNumcode), m_syndrome);

  m_rate += m_ldpc_coder->get_r(syndrome_size, m_ldpc_coder->get_n());
}

void icov::decoder::ChannelDecoder::_extract_numcodes(
    omnidirectional_layer::BlockPrediction::ePredType block_prediction,
    int bitplaneCount) {
  for (int i = 0; i < bitplaneCount; ++i) {
    _extract_numcode(i, block_prediction);
  }
}

void icov::decoder::ChannelDecoder::_extract_numcode(
    int bp,
    omnidirectional_layer::BlockPrediction::ePredType block_prediction) {

  byte quantizedNumcode =
      (block_prediction ==
       omnidirectional_layer::BlockPrediction::ePredType::NONE)
          ? bitstream::enum_t::SyndromeType::SyndromeType_SOURCE
          : m_bitstream->pop();

  bitstream::VideoStreamDebugger::get_const_instance().assertNumcodeEqual(
      bp, quantizedNumcode);

  m_numcodes.push(quantizedNumcode);
}

void icov::decoder::ChannelDecoder::_decode_bitplanes(
    omnidirectional_layer::BlockPrediction::ePredType block_prediction,
    int bitplaneCount) {
  m_bsp.reset_src();
  m_bsp.resetBitplanes(bitplaneCount);
  for (int i = 0; i < bitplaneCount; ++i) {
    _decode_bitplane(i, block_prediction);
  }
  m_rate /= bitplaneCount;
}

void ChannelDecoder::decode_icu(int icu_enc, omnidirectional_layer::BlockPrediction::ePredType block_prediction)
{

  m_rate = 0;

  ICOV_ASSERT_MSG(block_prediction != omnidirectional_layer::BlockPrediction::ePredType::UNKNOWN, "Prediction unknown")

  bool is_access_block =
      (block_prediction ==
       omnidirectional_layer::BlockPrediction::ePredType::NONE);
  bool is_inter_prediction =
      (block_prediction ==
       omnidirectional_layer::BlockPrediction::ePredType::INTER);
  bool is_intra_prediction = !is_access_block && !is_inter_prediction;

  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingChannelID(
      m_params.CompID);

  m_icuIndex = icu_enc;

  m_block_prediction = block_prediction;
  std::fill(m_blockSI.begin(), m_blockSI.end(), 0);
  std::fill(m_blockBitplane.begin(), m_blockBitplane.end(), 0);

  //  const auto comp_area_value = m_blockArea.area();

  if (!is_access_block) {
    ICOV_TRACE << "DECODE BLOCK WITH PRED "
               << omnidirectional_layer::BlockPrediction::toString(
                      block_prediction);
  } else {
    ICOV_TRACE << "DECODE ACCESS BLOCK";
  }

  int bitplaneCount = (int)m_bitstream->pop();
  ICOV_TRACE << " BP COUNT = " << bitplaneCount;

  bitstream::VideoStreamDebugger::get_const_instance().assertBitplaneCountEqual(
      bitplaneCount);

  m_bsp.distribution().setZmin(0);
  m_bsp.distribution().setZmax(0);
  m_bsp.distribution().setQ0(1);
  m_bsp.distribution().setBitplane_count(bitplaneCount);
  m_bsp.reset_src();
  m_bsp.set_si(m_blockSI);

  bool coding_flag = false;

  if (is_intra_prediction) {
    _extract_si_dir_mode();
    _extract_qary_params(coding_flag);
    _set_si(coding_flag);
  } else if (is_access_block) {
    coding_flag = true;
  }

  if (coding_flag)
  {
    _extract_numcodes(block_prediction, bitplaneCount);
    _decode_bitplanes(block_prediction, bitplaneCount);
  }

  end_decode_icu();
}

void ChannelDecoder::_extract_si_dir_mode() {
  byte dirmode = m_bitstream->pop();

  bitstream::assertPredictionAvailable(dirmode);

  bitstream::VideoStreamDebugger::get_const_instance().assertDirModeEqual(
      dirmode);

  dirmode = bitstream::getPredDirMode(dirmode);

  auto mp_icu_enc = m_encodingStruct->getICU(m_icuIndex);
  auto m_blockArea = mp_icu_enc.get_sourceX().block(m_params.CompID);

  auto roiComp_predictionFrameBuff =
      VTM_Utility::getAreaBuf(m_vvc_bufs->get(PIC_PREDICTION), m_blockArea);

  m_intraPred.initIntraPatternChType(m_vvc_bufs->getBuf(PIC_RECONSTRUCTION),
                                     m_block_prediction, m_blockArea,
                                     m_params.bit_depth, false);

  auto currICUintraMap = mp_icu_enc.get_intraSIsMap();
  auto &si = currICUintraMap.at(m_block_prediction);

  si.intraDir[m_params.CompID] = dirmode;
  m_intraPred.predIntraAng(roiComp_predictionFrameBuff, dirmode);

  ICOV_ASSERT_MSG(si.TUs.size() == 1,
                  "This version of ICOV only supports one TU per ICU");

  auto &curr_tu_si = si.TUs[0];

  const auto &predictionUnitBuf = m_vvc_bufs->get(PIC_PREDICTION);

  m_transform2D.transformNxN(
      curr_tu_si.getTransformCoeffs(m_params.CompID), predictionUnitBuf,
      curr_tu_si.block(m_params.CompID), m_params.bit_depth,
      m_params.maxLog2TrDynamicRange);

  m_quant.quant(curr_tu_si.getQuantizedTransformCoeffs(m_params.CompID),
                curr_tu_si.getTransformCoeffs(m_params.CompID), m_uiAbsSum,
                m_params.QpParam, SliceType::I_SLICE, m_params.bit_depth,
                m_params.maxLog2TrDynamicRange);
}

void icov::decoder::ChannelDecoder::_extract_qary_params(bool &codingFlag) {

  byte pred_hdr = m_bitstream->pop();

  codingFlag = bitstream::getCodingFlag(pred_hdr);

  bitstream::VideoStreamDebugger::get_const_instance().assertcodingSIEqual(
      codingFlag);

  if (codingFlag) {

    byte zmin = 0, zmax = 0;

    byte q0 = bitstream::getQaryQParam(pred_hdr);
    bitstream::VideoStreamDebugger::get_const_instance().assertQaryParamEqual(
        ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Q, q0);

    zmin = m_bitstream->pop();
    bitstream::VideoStreamDebugger::get_const_instance().assertQaryParamEqual(
        ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Z_MIN, zmin);

    zmax = m_bitstream->pop();
    bitstream::VideoStreamDebugger::get_const_instance().assertQaryParamEqual(
        ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Z_MAX, zmax);

    m_bsp.distribution().setExtractedParams(m_bsp.bitplane_count(), q0, zmin,
                                            zmax);

    ICOV_TRACE << "Q0 = " << m_bsp.distribution().getQ0();
    ICOV_TRACE << "ZMIN = " << m_bsp.distribution().getZmin();
    ICOV_TRACE << "ZMAX = " << m_bsp.distribution().getZmax();
  }
}

void ChannelDecoder::_set_si(bool coding_flag) {
  auto icu_enc = m_encodingStruct->getICU(m_icuIndex);
  auto currICUintraMap = icu_enc.get_intraSIsMap();
  const auto &si = currICUintraMap.at(m_block_prediction);

  const auto &si_tu = si.TUs[0];
  auto si_tuCoeff = si_tu.getQuantizedTransformCoeffs(m_params.CompID);

  // SI is perfect => clone it into dec_tu
  if (!coding_flag) {
    ICOV_TRACE << "SI is PERFECT --> CLONE";
    icov::video::TransformUnit &dec_tu = icu_enc.sourceX_TU(0);
    std::copy(si_tuCoeff.buf, si_tuCoeff.buf + m_ldpc_coder->get_n(),
              dec_tu.getQuantizedTransformCoeffs(m_params.CompID).buf);
  } else { /* else assign it in bitsymbolprovider */
    ICOV_TRACE << "SI -> BSP";
    m_blockSI.assign(si_tuCoeff.buf, si_tuCoeff.buf + m_ldpc_coder->get_n());
    m_bsp.set_si(m_blockSI);
    bitstream::VideoStreamDebugger::get_const_instance().assertSiVec(
        m_bsp.si_symboles());
  }
}

void ChannelDecoder::read_syndrome(int syndrome_size)
{
  m_syndrome.resize(syndrome_size);
  byte bitstream_value = 0;
  ICOV_TRACE << "READ SYNDROME " << syndrome_size;
  for (int i = 0; i < syndrome_size; ++i) {
    if ((i % BYTE_SIZE) == 0) {
      bitstream_value = m_bitstream->pop();
      // std::cout << " " << (int)bitstream_value;
    }
    m_syndrome[i] =
        ((bitstream_value & (1 << (BYTE_SIZE - 1 - i % BYTE_SIZE))) > 0);
  }
}

void ChannelDecoder::decode_bitplane_with_ldpc(int bp, NumcodeIdx numcode, const Syndrome &synd)
{
  if (m_block_prediction ==
      icov::omnidirectional_layer::BlockPrediction::ePredType::NONE) {
    ICOV_ASSERT_MSG(numcode ==
                        bitstream::enum_t::SyndromeType::SyndromeType_SOURCE,
                    "Numcode value must be set to 1 for access block");
  }

  bitstream::VideoStreamDebugger::get_const_instance().assertSyndromeEqual(
      bp, synd);

  m_ldpc_coder->decode_bitplane(m_blockBitplane, bp, m_bsp, numcode, synd);

  bitstream::VideoStreamDebugger::get_const_instance().assertBitplaneEqual(
      bp, m_bsp.src_bits().at(bp));

  auto mp_icu_enc = m_encodingStruct->getICU(m_icuIndex);

  if ((bp + 1) == m_bsp.bitplane_count()) {
    icov::video::TransformUnit &dec_tu = mp_icu_enc.sourceX_TU(0);
    std::copy(m_bsp.src_symboles().begin(), m_bsp.src_symboles().end(),
              dec_tu.getQuantizedTransformCoeffs(m_params.CompID).buf);
    //    icov::io::write_vector(std::cout, m_bsp.src_symboles(), ' ');
    //    std::cout << std::endl;
  }
}

void ChannelDecoder::end_decode_icu() {

  auto mp_icu_enc = m_encodingStruct->getICU(m_icuIndex);

  icov::video::TransformUnit &dec_tu = mp_icu_enc.sourceX_TU(0);

  const auto dec_buf = dec_tu.getQuantizedTransformCoeffs(m_params.CompID).buf;
  std::copy(dec_buf, dec_buf + m_blockSymbols.size(), m_blockSymbols.begin());
  bitstream::VideoStreamDebugger::get_const_instance().assertSymbolesEqual(
      m_blockSymbols);

  // UnitArea blockArea;
  // PelStorage dec_block;
  //  dec_block.create(blockArea);

  // iQ
  m_quant.dequant(dec_tu.getInverseQuantizedTransformCoeffs(m_params.CompID),
                  dec_tu.getQuantizedTransformCoeffs(m_params.CompID),
                  m_params.QpParam, m_params.bit_depth,
                  m_params.maxLog2TrDynamicRange);

  // iDCT
  m_transform2D.invTransformNxN(
      m_vvc_bufs->get(PIC_RECONSTRUCTION), dec_tu.block(m_params.CompID),
      dec_tu.getInverseQuantizedTransformCoeffs(m_params.CompID),
      m_params.bit_depth, m_params.maxLog2TrDynamicRange);

  m_block_prediction =
      icov::omnidirectional_layer::BlockPrediction::ePredType::UNKNOWN;
  m_icuIndex = -1;
}

icov::Syndrome &ChannelDecoder::syndrome() { return m_syndrome; }

const icov::ldpc_codec_layer::AffInteractiveCoder &
ChannelDecoder::ldpc_coder() const {
  return *m_ldpc_coder;
}

const ChannelDecoder::params &ChannelDecoder::get_params() const {
  return m_params;
}

Rate ChannelDecoder::rate() const
{
  return m_rate;
}
} // namespace decoder
} // namespace icov
