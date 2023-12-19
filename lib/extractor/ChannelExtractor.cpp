#include "ChannelExtractor.h"

#include "icov_assert.h"
#include "icov_exception.h"
#include "icov_log.h"
#include "ldpc_codec_layer/QaryDistribution.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "video_layer/ExtractedStreamDebugger.h"
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

icov::byte ChannelExtractor::SyndromePadding = 0;

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

  std::cout << std::to_string(m_params.CompID) << ": extract_block_syndrome "
            << std::to_string(b.getIdSelf()) << ", "
            << omnidirectional_layer::BlockPrediction::toString(
                   block_prediction)
            << std::endl;

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

  bool is_inter_prediction =
      (block_prediction ==
       omnidirectional_layer::BlockPrediction::ePredType::INTER);

  int pred_type = is_inter_prediction ? S_INTER : S_INTRA;

  m_bitstream->push(m_bitplaneCount);

  for (auto &bp : m_bitplanes) {
    _extractBitplaneSyndrome(bp, pred_type);
  }

  std::cout << "END extract_block_syndrome " << std::endl;
}

void ChannelExtractor::extract_stored_syndrome_size(bool is_access_block) {
  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingChannelID(
      m_params.CompID);

  std::cout << std::to_string(m_params.CompID)
            << ": extract_stored_syndrome_size "
            << std::to_string(is_access_block) << std::endl;

  // read bitplane count
  m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS,
                   m_bitplaneCount);

  bitstream::VideoStreamDebugger::get_const_instance().assertBitplaneCountEqual(
      m_bitplaneCount);

  std::cout << " m_bitplaneCount " << (int)m_bitplaneCount << "\n";

  m_bitplanes.resize(m_bitplaneCount);

  // for each bitplane read stored syndrome size
  int i = 0;
  for (auto &bp : m_bitplanes) {
    bp.Index = i++;

    if (bp.Index == 0) {
      for (int k = 0; k < PRED_COUNT; ++k) {
        bp.BitRates[k].reset();
        m_predParams[k].reset();
      }
    }

    byte qSize = 0;

    m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS, qSize);

    bitstream::VideoStreamDebugger::get_const_instance()
        .assertStoredNumcodeIdxEqual(bp.Index, qSize);

    if (is_access_block) {
      qSize = bitstream::enum_t::SyndromeType::SyndromeType_SOURCE;
    }

    bp.BitRates[S_STORED].set(qSize, *m_ldpc_coder, bp.ReadSyndromePadding);

    m_predParams[S_STORED].setSyndromeLength(
        m_predParams[S_STORED].getSyndromeLength() +
        bp.BitRates[S_STORED].syndromeBitSize());
  }
  std::cout << "END extract_stored_syndrome_size " << std::endl;
}

void ChannelExtractor::pass_access_block() {
  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingChannelID(
      m_params.CompID);

  std::cout << std::to_string(m_params.CompID) << ": pass_access_block "
            << std::endl;

  ushort syndrome_size = 0, syndrome_padding = 0;
  bitstream::getByteCount(m_ldpc_coder->get_n() * m_bitplaneCount -
                              SyndromePadding,
                          syndrome_size, syndrome_padding);
  m_icovFile->offsetSyndrome(syndrome_size);
  SyndromePadding =
      ((syndrome_padding == 0) ? (0) : (BYTE_SIZE - syndrome_padding));

  std::cout << "END pass_access_block " << std::endl;
}

void ChannelExtractor::pass_prediction() {
  bitstream::VideoStreamDebugger::get_mutable_instance().setDebuggingChannelID(
      m_params.CompID);
  m_icovFile->jumpPrediction(m_bitplaneCount);
}

void ChannelExtractor::resetSyndromePadding() { SyndromePadding = 0; }

int ChannelExtractor::getSyndromePadding() { return SyndromePadding; }

void ChannelExtractor::extractPredParams(int type) {
  _extractPredParams(m_predParams[type]);

  if (m_predParams[type].getCodingFlag()) {
    for (auto &bp : m_bitplanes) {
      _extractBitplaneNumcode(bp, type);
    }
  }
}

void ChannelExtractor::_extractPredParams(bitstream::SIEncodedParams &si) {
  ICOV_DEBUG << "CH " << std::to_string(m_params.CompID)
             << ": _extractPredParams " << std::endl;

  si.reset();

  byte dirmode = 0;
  m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS, dirmode);
  bitstream::assertPredictionAvailable(dirmode);
  bitstream::VideoStreamDebugger::get_const_instance().assertDirModeEqual(
      dirmode);

  ICOV_DEBUG << "EXTRACTOR DM ";
  byte pred_hdr = bitstream::flag_t::NoCodingSI;
  m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS, pred_hdr);
  ICOV_DEBUG << "EXTRACTOR QZ ";

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

  ICOV_DEBUG << "END _extractPredParams" << std::endl;
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
  ICOV_DEBUG << "CH " << std::to_string(m_params.CompID) << ": _pushPredParams "
             << std::endl;

  _pushPredParams(m_predParams[type]);

  if (m_predParams[type].getCodingFlag()) {
    for (auto &bp : m_bitplanes) {
      m_bitstream->push(bp.BitRates[type].syndromeQuantizedSize());
    }
  }

  ICOV_DEBUG << "END _pushPredParams" << std::endl;
}

void ChannelExtractor::_extractBitplaneNumcode(BitplaneSyndromeExtractor &bp,
                                               int pred_type) {
  std::cout << "CH " << std::to_string(m_params.CompID)
            << ": _extractBitplaneNumcode" << std::endl;

  byte qsize = 0;
  m_icovFile->read(bitstream::eIcovStream::ENCODE_PARAMS, qsize);
  bp.BitRates[pred_type].set(qsize, *m_ldpc_coder, bp.ReadSyndromePadding);

  bitstream::VideoStreamDebugger::get_const_instance().assertNumcodeEqual(
      bp.Index, (NumcodeIdx)bp.BitRates[pred_type].syndromeQuantizedSize());

  std::cout << "END _extractBitplaneNumcode" << std::endl;
}

void ChannelExtractor::_extractBitplaneSyndrome(
    const BitplaneSyndromeExtractor &bp, int pred_type) {

  ushort bytes_count_to_read = bp.BitRates[pred_type].syndromeByteSize();

  if (bytes_count_to_read > 0) {
    ushort bytes_count_stored = bp.BitRates[S_STORED].syndromeByteSize();
    ushort offset = bytes_count_stored - bytes_count_to_read;
    icov::assert::check_not(offset < 0, "Offset can't be negative");

    ICOV_TRACE << "CH " << std::to_string(m_params.CompID)
               << ": _extractBitplaneSyndrome";
    ICOV_TRACE << bytes_count_to_read << "/" << bytes_count_stored
               << " bytes to read";

    ICOV_TRACE << "SYNDROME POS IN ENCODED FILE START FROM "
               << m_icovFile->getStream(icov::bitstream::eIcovStream::SYNDROME)
                      .tellg();

    ICOV_TRACE << "BITSTREAM POS BEFORE READ "
               << m_bitstream->getWritePosition();

    int total_read = 0;

    while (total_read < bytes_count_to_read) {
      int read = std::min((int)(bytes_count_to_read - total_read),
                          (int)((m_bitstream->getBufferSize() -
                                 m_bitstream->getWritePosition())));

      m_icovFile->readSyndrome(m_bitstream->writableData(), read, read);
      m_bitstream->shiftWritePosition(read);

      total_read += read;
    }

    icov::assert::equal(total_read, (int)bytes_count_stored,
                        "Syndrome read size bad value");

    m_icovFile->offsetSyndrome(offset);

    ICOV_TRACE << "SYNDROME POS NOW IS "
               << m_icovFile->getStream(icov::bitstream::eIcovStream::SYNDROME)
                      .tellg();

    ICOV_TRACE << "BITSTREAM POS AFTER READ "
               << m_bitstream->getWritePosition();
  }
  std::cout << "END _extractBitplaneSyndrome" << std::endl;
}

const std::array<icov::bitstream::SIEncodedParams, PRED_COUNT> &
ChannelExtractor::predParams() const {
  return m_predParams;
}

int ChannelExtractor::getCoderSize() const { return m_ldpc_coder->get_n(); }
