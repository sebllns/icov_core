#include "ChannelExtractor.h"

#include "icov_assert.h"
#include "icov_exception.h"
#include "icov_log.h"
#include "ldpc_codec_layer/QaryDistribution.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "ldpc_codec_layer/interface/IInteractiveCoder.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/VideoMetrics.h"
#include "video_layer/encoded_stream.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"

#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <algorithm>
#include <string>
#include <sys/types.h>

namespace fs = boost::filesystem;
using namespace icov::extractor;
using SI_SIDE = icov::omnidirectional_layer::BlockPrediction::ePredType;

ChannelExtractor::ChannelExtractor(
    const params &p, std::shared_ptr<InputIcov> &icovFile,
    const bitstream::sp_IcovExtractedBitstream &bitstream,
    const LdpcCoderParameters &LDPC_params)
    : m_params(p),
      m_ldpc_coder(
          icov::ldpc_codec_layer::LdpcCoderFactory::create_coder(LDPC_params)),
      m_icovFile(icovFile), m_bitstream(bitstream),
      m_maxSyndromeBufferLength(
          bitstream::bitToByteCount(m_ldpc_coder->get_m_max()))
/*,m_readingBuffer(m_maxSyndromeBufferLength)*/ {
  m_bitplanes.reserve(MAX_BITPLANE_COUNT);
  //  m_extractData.BitplaneNumcodes.reset(MAX_BITPLANE_COUNT);

  // m_extractData.SyndromeBuf.reset(m_maxSyndromeBufferLength);
}

ChannelExtractor::~ChannelExtractor() {}

void ChannelExtractor::extract_block_syndrome(
    const omnidirectional_layer::Block &b,
    omnidirectional_layer::BlockPrediction::ePredType block_prediction) {

  ICOV_TRACE << std::to_string(m_params.CompID) << ": extract_block_syndrome "
             << std::to_string(b.getIdSelf()) << ", "
             << omnidirectional_layer::BlockPrediction::toString(
                    block_prediction);

  ICOV_ASSERT_MSG(
      block_prediction !=
          icov::omnidirectional_layer::BlockPrediction::ePredType::UNKNOWN,
      "Prediction unknown");

  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingBlockID(
      b.getIdSelf() - 1);
  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingChannelID(
      m_params.CompID);
  bitstream::VideoStreamDebugger::get_mutable_instance()
      .setDebuggingPredictionID(block_prediction);

  bool is_access_block =
      (block_prediction ==
       omnidirectional_layer::BlockPrediction::ePredType::NONE);

  bool is_inter_prediction =
      (block_prediction ==
       omnidirectional_layer::BlockPrediction::ePredType::INTER);

  int pred_type =
      is_access_block ? S_STORED : (is_inter_prediction ? S_INTER : S_INTRA);

  m_bitstream->push(m_bitplaneCount);

  if (!is_access_block)
    _pushPredParams(pred_type);

  for (auto &bp : m_bitplanes) {
    _extractBitplaneSyndrome(bp, pred_type);
  }

  ICOV_TRACE << "END extract_block_syndrome ";
}

void ChannelExtractor::extract_stored_syndrome_size(bool is_access_block) {
  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingChannelID(
      m_params.CompID);

  ICOV_TRACE << std::to_string(m_params.CompID)
             << ": extract_stored_syndrome_size "
             << std::to_string(is_access_block);

  // read bitplane count
  m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS,
                   m_bitplaneCount);

  bitstream::VideoStreamDebugger::get_const_instance().assertBitplaneCountEqual(
      m_bitplaneCount);

  ICOV_TRACE << " m_bitplaneCount " << (int)m_bitplaneCount;

  m_bitplanes.resize(m_bitplaneCount);

  // for each bitplane read stored syndrome size
  int i = 0;
  for (auto &bp : m_bitplanes) {
    bp.Index = i++;

    for (int k = 0; k < PRED_COUNT; ++k) {
      bp.BitRates[k].reset();

      if (bp.Index == 0)
        m_predParams[k].reset();
    }

    byte qSize = 0;
    ushort padding = 0;

    m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS, qSize);

    bitstream::VideoStreamDebugger::get_const_instance()
        .assertStoredNumcodeIdxEqual(bp.Index, qSize);

    if (is_access_block) {
      qSize = ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE;
    }

    bp.BitRates[S_STORED].set(qSize, *m_ldpc_coder, padding);

    m_predParams[S_STORED].setSyndromeLength(
        m_predParams[S_STORED].getSyndromeLength() +
        bp.BitRates[S_STORED].syndromeBitSize());
  }
  ICOV_TRACE << "END extract_stored_syndrome_size ";
}

void ChannelExtractor::pass_access_block() {
  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingChannelID(
      m_params.CompID);

  ICOV_TRACE << std::to_string(m_params.CompID) << ": pass_access_block ";

  ushort syndrome_size = 0, syndrome_padding = 0;
  bitstream::getByteCount(m_ldpc_coder->get_n() * m_bitplaneCount,
                          syndrome_size, syndrome_padding);
  m_icovFile->offsetSyndrome(syndrome_size);

  ICOV_TRACE << "END pass_access_block ";
}

void ChannelExtractor::pass_prediction() {
  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingChannelID(
      m_params.CompID);
  m_icovFile->jumpPrediction(m_bitplaneCount);
}

void ChannelExtractor::extractPredParams(int type) {
  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingChannelID(
      m_params.CompID);
  _extractPredParams(m_predParams[type]);
  if (m_predParams[type].getCodingFlag()) {
    for (auto &bp : m_bitplanes) {
      _extractBitplaneNumcode(bp, type);
    }
  }
}

void ChannelExtractor::_extractPredParams(bitstream::SIEncodedParams &si) {
  ICOV_TRACE << "CH " << std::to_string(m_params.CompID)
             << ": _extractPredParams " << std::endl;

  si.reset();

  byte dirmode = 0;
  m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS, dirmode);

  ICOV_TRACE << "EXTRACTOR DM " << (int)dirmode;

  bitstream::assertPredictionAvailable(dirmode);
  bitstream::VideoStreamDebugger::get_const_instance().assertDirModeEqual(
      dirmode);

  byte pred_hdr = bitstream::flag_t::NoCodingSI;
  m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS, pred_hdr);
  ICOV_TRACE << "EXTRACTOR QZ ";

  si.setMode(dirmode);
  si.setPredictionHeader(pred_hdr);
  si.setCodingFlag(bitstream::getCodingFlag(pred_hdr));

  bitstream::VideoStreamDebugger::get_const_instance().assertcodingSIEqual(
      si.getCodingFlag());

  if (si.getCodingFlag()) {

    byte q0 = bitstream::getQaryQParam(pred_hdr);
    bitstream::VideoStreamDebugger::get_const_instance().assertQaryParamEqual(
        ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Q, q0);

    byte zmin = 0, zmax = 0;

    m_icovFile->read(bitstream::eIcovStream::ENCODE_PARAMS, zmin);
    bitstream::VideoStreamDebugger::get_const_instance().assertQaryParamEqual(
        ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Z_MIN, zmin);

    m_icovFile->read(bitstream::eIcovStream::ENCODE_PARAMS, zmax);
    bitstream::VideoStreamDebugger::get_const_instance().assertQaryParamEqual(
        ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Z_MAX, zmax);

    si.setZmin(zmin);
    si.setZmax(zmax);
  }

  ICOV_TRACE << "MODE = " << (int)si.getMode();
  ICOV_TRACE << "FLAG = " << si.getCodingFlag();
  ICOV_TRACE << "PRED_HDR = " << (int)si.getPredictionHeader();
  ICOV_TRACE << "Z_MIN = " << (int)si.getZmin();
  ICOV_TRACE << "Z_MAX = " << (int)si.getZmax();

  ICOV_TRACE << "END _extractPredParams" << std::endl;
}

void ChannelExtractor::_pushPredParams(const bitstream::SIEncodedParams &si) {
  m_bitstream->push(si.getMode());
  m_bitstream->push(si.getPredictionHeader());

  if (si.getCodingFlag()) {
    m_bitstream->push(si.getZmin());
    m_bitstream->push(si.getZmax());
  }
}

void ChannelExtractor::_pushPredParams(int type) {
  ICOV_TRACE << "CH " << std::to_string(m_params.CompID) << ": _pushPredParams "
             << std::endl;

  _pushPredParams(m_predParams[type]);

  if (m_predParams[type].getCodingFlag()) {
    for (auto &bp : m_bitplanes) {
      m_bitstream->push(bp.BitRates[type].syndromeQuantizedSize());
    }
  }

  ICOV_TRACE << "END _pushPredParams" << std::endl;
}

void ChannelExtractor::_extractBitplaneNumcode(BitplaneSyndromeExtractor &bp,
                                               int pred_type) {

  byte qsize = 0;
  ushort padding = 0;

  // read syndrome quantized size value
  m_icovFile->read(bitstream::eIcovStream::ENCODE_PARAMS, qsize);

  // inverse size quantization and store the value
  bp.BitRates[pred_type].set(qsize, *m_ldpc_coder, padding);

  ICOV_TRACE << "CH " << std::to_string(m_params.CompID) << " BP " << bp.Index
             << " BP_NUMCODE " << bp.BitRates[pred_type].toString();

  // debug check if enabled
  bitstream::VideoStreamDebugger::get_const_instance().assertNumcodeEqual(
      bp.Index, (NumcodeIdx)bp.BitRates[pred_type].syndromeQuantizedSize());

  if (pred_type == S_INTER &&
      qsize == ldpc_codec_layer::IInteractiveCoder::SyndromeType_SOURCE &&
      bp.BitRates[S_STORED].syndromeQuantizedSize() != qsize) {
    m_predParams[pred_type].setEnabledFlag(false);
  }

  ushort bitsize = bp.BitRates[pred_type].syndromeBitSize();

  // accumulate syndrome total length for the prediction type
  m_predParams[pred_type].setSyndromeLength(
      m_predParams[pred_type].getSyndromeLength() + bitsize);
}

void ChannelExtractor::_extractBitplaneSyndrome(
    const BitplaneSyndromeExtractor &bp, int pred_type) {

  ushort bytes_count_to_read = bp.BitRates[pred_type].syndromeByteSize();
  ushort bytes_count_stored = bp.BitRates[S_STORED].syndromeByteSize();

  if (bytes_count_stored < bytes_count_to_read) {
    ICOV_ERROR << "Channel " << (int)m_params.CompID;
    ICOV_ERROR << "Prediction " << (pred_type == S_INTER ? "INTER" : "INTRA");
    ICOV_ERROR
        << "Trying to read " << bytes_count_to_read << " bytes while "
        << bytes_count_stored
        << " bytes are stored in prediction"; /*<<
                                                omnidirectional_layer::BlockPrediction::toString(m_block_prediction)*/

    ICOV_THROW_OUT_OF_RANGE("Syndrome overflow");
  }

  ICOV_TRACE << "CH " << std::to_string(m_params.CompID)
             << ": _extractBitplaneSyndrome";

  ICOV_TRACE << bytes_count_to_read << "/" << bytes_count_stored
             << " bytes to read";

  ushort offset = bytes_count_stored - bytes_count_to_read;
  icov::assert::check_not(offset < 0,
                          ICOV_DEBUG_HEADER("Offset can't be negative"));

  ICOV_TRACE
      << "SYNDROME POS IN ENCODED FILE START FROM "
      << m_icovFile->getStream(icov::bitstream::eIcovStream::SYNDROME).tellg();

  ICOV_TRACE << "BITSTREAM POS BEFORE READ " << m_bitstream->getWritePosition();

  int total_read = 0;

  if (bytes_count_to_read > 0) {
    while (total_read < bytes_count_to_read) {
      int read = std::min((int)(bytes_count_to_read - total_read),
                          (int)((m_bitstream->getBufferSize() -
                                 m_bitstream->getWritePosition())));

      m_icovFile->readSyndrome(m_bitstream->writableData(), read, read);
      m_bitstream->shiftWritePosition(read);

      total_read += read;
    }
  }

  icov::assert::equal(
      total_read, (int)bytes_count_to_read,
      ICOV_DEBUG_HEADER("Check syndrome read size consistency"));

  m_icovFile->offsetSyndrome(offset);

  ICOV_DEBUG
      << "SYNDROME POS NOW IS "
      << m_icovFile->getStream(icov::bitstream::eIcovStream::SYNDROME).tellg();

  ICOV_TRACE << "BITSTREAM POS AFTER READ " << m_bitstream->getWritePosition();

  ICOV_TRACE << "END _extractBitplaneSyndrome";
}

const std::array<icov::bitstream::SIEncodedParams, PRED_COUNT> &
ChannelExtractor::predParams() const {
  return m_predParams;
}

int ChannelExtractor::getCoderSize() const { return m_ldpc_coder->get_n(); }

int ChannelExtractor::getBitplaneCount() const { return m_bitplaneCount; }
