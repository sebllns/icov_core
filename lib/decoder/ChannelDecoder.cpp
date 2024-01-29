#include "ChannelDecoder.h"
#include "FastArray.h"
#include "Unit.h"

#include "icov_assert.h"
#include "icov_exception.h"
#include "icov_log.h"
#include "ldpc_codec_layer/BitSymbolProvider.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/ICOV_Quant.h"
#include "video_layer/ICOV_Transform2d.h"
#include "video_layer/VideoMetrics.h"
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
      m_blockSymbols(m_ldpc_coder->get_n(), 0)
{
    m_intraPred.init(m_params.QpParam.chromaFomat);
}

ChannelDecoder::~ChannelDecoder() {}

void icov::decoder::ChannelDecoder::_decode_bitplane(
    int i, omnidirectional_layer::BlockPrediction::ePredType block_prediction) {

    ICOV_DEBUG << ICOV_DEBUG_HEADER("_decode_bitplane ") << i;

    byte quantizedNumcode = m_numcodes.front();
    m_numcodes.pop();

    ICOV_DEBUG << "READ NUMCODE " << (int)quantizedNumcode;

    bitstream::VideoStreamDebugger::get_const_instance().assertNumcodeEqual(
        i, quantizedNumcode);

    int syndrome_size =
        bitstream::inverseQuantizedSize(quantizedNumcode, *m_ldpc_coder);

    read_syndrome(syndrome_size);

    //    icov::video_layer::VideoMetrics::get_mutable_instance()
    //        .metrics[m_icuIndex]
    //        .encodedBitlength += syndrome_size;

    decode_bitplane_with_ldpc(
        i, static_cast<icov::NumcodeIdx>(quantizedNumcode), m_syndrome);
}

void icov::decoder::ChannelDecoder::_extract_numcodes(
    omnidirectional_layer::BlockPrediction::ePredType block_prediction,
    int bitplaneCount) {
    ICOV_DEBUG << ICOV_DEBUG_HEADER("_extract_numcodes");
    for (int i = 0; i < bitplaneCount; ++i) {
      _extract_numcode(i, block_prediction);
    }
}

void icov::decoder::ChannelDecoder::_extract_numcode(
    int bp,
    omnidirectional_layer::BlockPrediction::ePredType block_prediction) {
    ICOV_DEBUG << ICOV_DEBUG_HEADER("_extract_numcode");

    byte quantizedNumcode =
        (block_prediction ==
         omnidirectional_layer::BlockPrediction::ePredType::NONE)
            ? ldpc_codec_layer::IInteractiveCoder::SyndromeType::
                  SyndromeType_SOURCE
            : m_bitstream->pop();

    bitstream::VideoStreamDebugger::get_const_instance().assertNumcodeEqual(
        bp, quantizedNumcode);

    m_numcodes.push(quantizedNumcode);
}

void icov::decoder::ChannelDecoder::_decode_bitplanes(
    omnidirectional_layer::BlockPrediction::ePredType block_prediction,
    int bitplaneCount) {
    ICOV_DEBUG << ICOV_DEBUG_HEADER("_decode_bitplanes");

    m_bsp.reset_src();
    m_bsp.resetBitplanes(bitplaneCount);
    for (int i = 0; i < bitplaneCount; ++i) {
      _decode_bitplane(i, block_prediction);
    }
}

void ChannelDecoder::decode_icu(
    int icu_enc,
    omnidirectional_layer::BlockPrediction::ePredType block_prediction) {

    ICOV_DEBUG << ICOV_DEBUG_HEADER("decode_icu");

    m_block_prediction = block_prediction;

    icov::assert::not_equal(
        m_block_prediction,
        omnidirectional_layer::BlockPrediction::ePredType::UNKNOWN,
        ICOV_DEBUG_HEADER("Prediction unknown"));

    bool is_access_block =
        (m_block_prediction ==
         omnidirectional_layer::BlockPrediction::ePredType::NONE);

    bitstream::VideoStreamDebugger::get_mutable_instance()
        .setDebuggingChannelID(m_params.CompID);

    m_icuIndex = icu_enc;

    std::fill(m_blockSI.begin(), m_blockSI.end(), 0);
    std::fill(m_blockBitplane.begin(), m_blockBitplane.end(), 0);

    //  const auto comp_area_value = m_blockArea.area();

    if (!is_access_block) {
      ICOV_TRACE << ICOV_DEBUG_HEADER("DECODE BLOCK WITH PRED ")
                 << omnidirectional_layer::BlockPrediction::toString(
                        block_prediction);
    } else {
      ICOV_TRACE << ICOV_DEBUG_HEADER("DECODE ACCESS BLOCK");
    }

    int bitplaneCount = (int)m_bitstream->pop();
    ICOV_TRACE << ICOV_DEBUG_HEADER(" BP COUNT = ") << bitplaneCount;

    bitstream::VideoStreamDebugger::get_const_instance()
        .assertBitplaneCountEqual(bitplaneCount);

    m_bsp.distribution().setZmin(0);
    m_bsp.distribution().setZmax(0);
    m_bsp.distribution().setQ0(1);
    m_bsp.distribution().setBitplane_count(bitplaneCount);
    m_bsp.reset_src();
    m_bsp.set_si(m_blockSI);

    coding_flag = true;

    if (!is_access_block) {
      _set_pred_mode(_extract_pred_mode());
    }

    //    icov::video_layer::VideoMetrics::get_mutable_instance()
    //        .metrics[m_icuIndex]
    //        .decodedBitlength += bitplaneCount * m_ldpc_coder->get_n();

    // USE LDPC
    if (coding_flag) {
      _extract_numcodes(m_block_prediction, bitplaneCount);
      _decode_bitplanes(m_block_prediction, bitplaneCount);
    }

    // do nothing if block is exaclty the same as in the previous frame
    if (coding_flag ||
        m_block_prediction !=
            omnidirectional_layer::BlockPrediction::ePredType::INTER) {
      inverse_QDCT();
    } else {
      ICOV_DEBUG << ICOV_DEBUG_HEADER(
          "No inverse QDCT since temporal coding flag is false");
    }

    m_block_prediction =
        icov::omnidirectional_layer::BlockPrediction::ePredType::UNKNOWN;
    m_icuIndex = -1;
}

ushort ChannelDecoder::_extract_pred_mode() {
    ICOV_DEBUG << ICOV_DEBUG_HEADER("_extract_pred_mode");

    byte dirmode = m_bitstream->pop();
    bitstream::assertPredictionAvailable(dirmode);
    return bitstream::getPredDirMode(dirmode);
}

void ChannelDecoder::_set_pred_mode(ushort dirmode) {
    ICOV_DEBUG << ICOV_DEBUG_HEADER("_set_pred_mode");

    // this special mode value is a flag to activate the temporal prediction
    // instead of the intra prediction
    if (dirmode == NUM_LUMA_MODE) {
      // replace current prediction by inter mode
      m_block_prediction =
          omnidirectional_layer::BlockPrediction::ePredType::INTER;
    }

    bitstream::VideoStreamDebugger::get_mutable_instance()
        .setDebuggingPredictionID(m_block_prediction);
    bitstream::VideoStreamDebugger::get_const_instance().assertDirModeEqual(
        bitstream::getPredDirModeByteValue(true, dirmode));

    auto si_cat =
        omnidirectional_layer::BlockPrediction::getCategory(m_block_prediction);

    if (si_cat ==
        omnidirectional_layer::BlockPrediction::eSICategory::SI_CAT_TEMPORAL) {
      ICOV_DEBUG << ICOV_DEBUG_HEADER("DECODE INTER");
      // Mode INTER: use the same block data (decode from previous frame)

      _extract_qary_params(coding_flag);
      _inter_to_bsp(coding_flag);

    } else if (si_cat == omnidirectional_layer::BlockPrediction::eSICategory::
                             SI_CAT_SINGLE ||
               si_cat == omnidirectional_layer::BlockPrediction::eSICategory::
                             SI_CAT_DOUBLE) {
      ICOV_DEBUG << ICOV_DEBUG_HEADER("DECODE INTRA");
      // Mode INTRA: use the neighbor block(s) (decode from current frame)
      _apply_intra_dir_mode(dirmode);
      _extract_qary_params(coding_flag);
      _intra_to_bsp(coding_flag);
    } else {
      ICOV_THROW_LOGICAL_EXCEPTION(
          "Prediction is not valid side info value: " +
          omnidirectional_layer::BlockPrediction::toString(m_block_prediction));
    }
}

void ChannelDecoder::_apply_intra_dir_mode(ushort dirmode) {
    ICOV_DEBUG << ICOV_DEBUG_HEADER("_apply_intra_dir_mode");

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

    ICOV_ASSERT_MSG(
        si.TUs.size() == 1,
        ICOV_DEBUG_HEADER("This version of ICOV only supports one TU per ICU"));

    auto &curr_tu_si = si.TUs[0];

    const auto &predictionUnitBuf = m_vvc_bufs->get(PIC_PREDICTION);

    ICOV_DEBUG << ICOV_DEBUG_HEADER("INTRA DCT-Q ") << " ch=" << m_params.CompID
               << ", qp=" << m_params.QpParam.Qp
               << ", bd=" << m_params.bit_depth
               << ", ml=" << m_params.maxLog2TrDynamicRange;

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
    ICOV_DEBUG << ICOV_DEBUG_HEADER("_extract_qary_params");

    byte pred_hdr = m_bitstream->pop();

    codingFlag = bitstream::getCodingFlag(pred_hdr);

    bitstream::VideoStreamDebugger::get_const_instance().assertcodingSIEqual(
        codingFlag);

    ICOV_DEBUG << ICOV_DEBUG_HEADER("coding_flag=" << codingFlag);

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

      ICOV_DEBUG << "Q0 = " << m_bsp.distribution().getQ0();
      ICOV_DEBUG << "ZMIN = " << m_bsp.distribution().getZmin();
      ICOV_DEBUG << "ZMAX = " << m_bsp.distribution().getZmax();
    }
}

void ChannelDecoder::_inter_to_bsp(bool coding_flag) {
    ICOV_DEBUG << ICOV_DEBUG_HEADER("_inter_to_bsp");

    if (coding_flag) {
      ICOV_TRACE << "INTER SI -> BSP";

      auto mp_icu_enc = m_encodingStruct->getICU(m_icuIndex);
      const auto &si_tu = mp_icu_enc.sourceX_TU(0);
      auto si_tuCoeff = si_tu.getQuantizedTransformCoeffs(m_params.CompID);

      m_blockSI.assign(si_tuCoeff.buf, si_tuCoeff.buf + m_ldpc_coder->get_n());
      m_bsp.set_si(m_blockSI);
      bitstream::VideoStreamDebugger::get_const_instance().assertSiVec(
          m_bsp.si_symboles());
    }
}

void ChannelDecoder::_intra_to_bsp(bool coding_flag) {
    ICOV_DEBUG << ICOV_DEBUG_HEADER("_intra_to_bsp");

    auto icu_enc = m_encodingStruct->getICU(m_icuIndex);
    auto currICUintraMap = icu_enc.get_intraSIsMap();
    const auto &si = currICUintraMap.at(m_block_prediction);

    const auto &si_tu = si.TUs[0];
    auto si_tuCoeff = si_tu.getQuantizedTransformCoeffs(m_params.CompID);

    // SI is perfect => clone it into dec_tu
    if (!coding_flag) {
      ICOV_TRACE << "INTRA SI is PERFECT --> CLONE";
      icov::video::TransformUnit &dec_tu = icu_enc.sourceX_TU(0);
      std::copy(si_tuCoeff.buf, si_tuCoeff.buf + m_ldpc_coder->get_n(),
                dec_tu.getQuantizedTransformCoeffs(m_params.CompID).buf);
    } else { /* else assign it in bitsymbolprovider */
      ICOV_TRACE << "INTRA SI -> BSP";
      m_blockSI.assign(si_tuCoeff.buf, si_tuCoeff.buf + m_ldpc_coder->get_n());
      m_bsp.set_si(m_blockSI);
      bitstream::VideoStreamDebugger::get_const_instance().assertSiVec(
          m_bsp.si_symboles());
    }
}

void ChannelDecoder::read_syndrome(int syndrome_size) {
    ICOV_DEBUG << ICOV_DEBUG_HEADER("read_syndrome");

    m_syndrome.resize(syndrome_size);
    byte bitstream_value = 0;
    ICOV_DEBUG << "READ SYNDROME " << syndrome_size;
    for (int i = 0; i < syndrome_size; ++i) {
      if ((i % BYTE_SIZE) == 0) {
        bitstream_value = m_bitstream->pop();
        ICOV_TRACE << (int)bitstream_value;
      }
      m_syndrome[i] =
          ((bitstream_value & (1 << (BYTE_SIZE - 1 - i % BYTE_SIZE))) > 0);
    }
}

void ChannelDecoder::decode_bitplane_with_ldpc(int bp, NumcodeIdx numcode,
                                               const Syndrome &synd) {
    ICOV_DEBUG << ICOV_DEBUG_HEADER("decode_bitplane_with_ldpc");

    if (m_block_prediction ==
        icov::omnidirectional_layer::BlockPrediction::ePredType::NONE) {
      icov::assert::check(
          numcode == ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE,
          ICOV_DEBUG_HEADER("Numcode source flag bad value for access block"));
    }

    if (m_ldpc_coder->inverseQuantizedSize(numcode) != (int)synd.size()) {
      ICOV_ERROR << "Syndrome size " << (int)synd.size();
      ICOV_ERROR << "Numcode " << (int)numcode << "("
                 << (int)m_ldpc_coder->inverseQuantizedSize(numcode) << ")";

      ICOV_THROW_LOGICAL_EXCEPTION(
          "Syndrome size does not match numcode value");
    }

    bitstream::VideoStreamDebugger::get_const_instance().assertNumcodeEqual(
        bp, numcode);
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

void ChannelDecoder::inverse_QDCT() {
    ICOV_DEBUG << ICOV_DEBUG_HEADER("inverse_QDCT");

    auto mp_icu_enc = m_encodingStruct->getICU(m_icuIndex);

    icov::video::TransformUnit &dec_tu = mp_icu_enc.sourceX_TU(0);

    const auto dec_buf =
        dec_tu.getQuantizedTransformCoeffs(m_params.CompID).buf;
    std::copy(dec_buf, dec_buf + m_blockSymbols.size(), m_blockSymbols.begin());
    bitstream::VideoStreamDebugger::get_const_instance().assertSymbolesEqual(
        m_blockSymbols);

    // UnitArea blockArea;
    // PelStorage dec_block;
    //  dec_block.create(blockArea);

    ICOV_DEBUG << ICOV_DEBUG_HEADER("IDCT-Q ") << " ch=" << m_params.CompID
               << ", qp=" << m_params.QpParam.Qp
               << ", bd=" << m_params.bit_depth
               << ", ml=" << m_params.maxLog2TrDynamicRange;

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
}

icov::Syndrome &ChannelDecoder::syndrome() { return m_syndrome; }

const icov::ldpc_codec_layer::AffInteractiveCoder &
ChannelDecoder::ldpc_coder() const {
  return *m_ldpc_coder;
}

const ChannelDecoder::params &ChannelDecoder::get_params() const {
  return m_params;
}

} // namespace decoder
} // namespace icov
