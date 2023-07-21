#include "ChannelExtractor.h"

#include "FastArray.h"

#include "icov_assert.h"
#include "icov_log.h"
#include "ldpc_codec_layer/QaryDistribution.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/Block.h"
#include "omnidirectional_layer/BlockPredictionManager.h"
#include "video_layer/ExtractedStreamDebugger.h"
#include "video_layer/encoded_stream.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"

#include <algorithm>
#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <string>
#include <sys/types.h>

namespace fs = boost::filesystem;
using namespace icov::extractor;
using SI_SIDE = icov::omnidirectional_layer::BlockPrediction::ePredType;

icov::bitstream::byte ChannelExtractor::SyndromePadding = 0;

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

  bool is_access_block =
      (block_prediction ==
       omnidirectional_layer::BlockPrediction::ePredType::NONE);
  bool is_inter_prediction =
      (block_prediction ==
       omnidirectional_layer::BlockPrediction::ePredType::INTER);
  bool is_intra_prediction = !is_access_block && !is_inter_prediction;

  m_bitstream->push(m_bitplaneCount);

  m_codingFlag = true;

  if (is_access_block) {
    ushort syndrome_size = 0, syndrome_padding = 0;
    //    bitstream::getByteCount(m_ldpc_coder->get_n() * m_bitplaneCount -
    //                                SyndromePadding,
    //                            syndrome_size, syndrome_padding);

    //    m_icovFile->readSyndrome(m_readingBuffer, syndrome_size,
    //    syndrome_size); SyndromePadding =
    //        ((syndrome_padding == 0) ? (0) : (BYTE_SIZE - syndrome_padding));

    m_codingFlag = true;
  } else if (is_intra_prediction) {
    _extractIntraParams();
  } else if (is_inter_prediction) {
    // TODO IMPLEMENT
  }

  // if (m_codingFlag) {
  for (auto &bp : m_bitplanes) {
    _extractBitplaneSyndrome(bp);
  }
  // }

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

  int i = 0;
  // for each bitplane read stored syndrome size
  for (auto &bp : m_bitplanes) {
    bp.Index = i++;
    bp.StoredSyndromeQuantizedSize = 0;
    bp.StoredSyndromeBitSize = 0;
    bp.StoredSyndromeByteSize = 0;

    bp.ReadSyndromeQuantizedSize = 0;
    bp.ReadSyndromeBitSize = 0;
    bp.ReadSyndromeByteSize = 0;
    bp.ReadSyndromePadding = 0;

    m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS,
                     bp.StoredSyndromeQuantizedSize);

    bitstream::VideoStreamDebugger::get_const_instance()
        .assertStoredNumcodeIdxEqual(bp.Index, bp.StoredSyndromeQuantizedSize);

    if (is_access_block) {
      bp.StoredSyndromeQuantizedSize =
          bitstream::enum_t::SyndromeType::SyndromeType_SOURCE;
    }

    bp.StoredSyndromeBitSize = bitstream::inverseQuantizedSize(
        bp.StoredSyndromeQuantizedSize, *m_ldpc_coder);

    bitstream::getByteCount(bp.StoredSyndromeBitSize, bp.StoredSyndromeByteSize,
                            bp.ReadSyndromePadding);
    bp.ReadSyndromeQuantizedSize = bp.StoredSyndromeQuantizedSize;
    bp.ReadSyndromeBitSize = bp.StoredSyndromeBitSize;
    bp.ReadSyndromeByteSize = bp.StoredSyndromeByteSize;
    // std::cout << " " << output[c][bp];
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

void ChannelExtractor::_extractIntraParams() {
  std::cout << "CH " << std::to_string(m_params.CompID)
            << ": _extractIntraParams " << std::endl;

  bitstream::byte dirmode = 0;
  m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS, dirmode);

  bitstream::assertPredictionAvailable(dirmode);

  bitstream::VideoStreamDebugger::get_const_instance().assertDirModeEqual(
      dirmode);

  ICOV_TRACE << "EXTRACTOR DM ";
  m_bitstream->push(dirmode);
  bitstream::byte pred_hdr = bitstream::flag_t::NoCodingSI;
  m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS, pred_hdr);
  ICOV_TRACE << "EXTRACTOR QZ ";
  m_bitstream->push(pred_hdr);

  m_codingFlag = bitstream::getCodingFlag(pred_hdr);
  bitstream::VideoStreamDebugger::get_const_instance().assertcodingSIEqual(
      m_codingFlag);

  if (m_codingFlag) {
    bitstream::byte zmin = 0, zmax = 0;

    bitstream::byte q0 = bitstream::getQaryQParam(pred_hdr);
    bitstream::VideoStreamDebugger::get_const_instance().assertQaryParamEqual(
        ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Q, q0);

    m_icovFile->read(bitstream::eIcovStream::ENCODE_PARAMS, zmin);
    bitstream::VideoStreamDebugger::get_const_instance().assertQaryParamEqual(
        ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Z_MIN, zmin);

    ICOV_TRACE << "EXTRACTOR ZMIN ";
    m_bitstream->push(zmin);

    m_icovFile->read(bitstream::eIcovStream::ENCODE_PARAMS, zmax);
    bitstream::VideoStreamDebugger::get_const_instance().assertQaryParamEqual(
        ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Z_MAX, zmax);

    ICOV_TRACE << "EXTRACTOR ZMAX ";
    m_bitstream->push(zmax);
  }

  for (auto &bp : m_bitplanes) {
    _extractBitplaneNumcode(bp);
  }

  std::cout << "END _extractIntraParams" << std::endl;
}

void ChannelExtractor::_extractBitplaneNumcode(BitplaneExtractor &bp) {
  std::cout << "CH " << std::to_string(m_params.CompID)
            << ": _extractBitplaneNumcode" << std::endl;

  if (m_codingFlag) {
    m_icovFile->read(bitstream::eIcovStream::ENCODE_PARAMS,
                     bp.ReadSyndromeQuantizedSize);

    bitstream::VideoStreamDebugger::get_const_instance().assertNumcodeEqual(
        bp.Index, (NumcodeIdx)bp.ReadSyndromeQuantizedSize);

    ICOV_TRACE << "EXTRACTOR NUMCODE BP " << bp.Index << " ";
    m_bitstream->push(bp.ReadSyndromeQuantizedSize);
  } else {
    bp.ReadSyndromeQuantizedSize = 0;
  }

  bp.ReadSyndromeBitSize = bitstream::inverseQuantizedSize(
      bp.ReadSyndromeQuantizedSize, *m_ldpc_coder);

  bitstream::getByteCount(bp.ReadSyndromeBitSize, bp.ReadSyndromeByteSize,
                          bp.ReadSyndromePadding);

  ICOV_TRACE << "STORED " << (int)bp.StoredSyndromeQuantizedSize << " Qbits -> "
             << bp.StoredSyndromeByteSize << " Bytes";
  ICOV_TRACE << "READ " << (int)bp.ReadSyndromeQuantizedSize << " Qbits -> "
             << bp.ReadSyndromeByteSize << " Bytes";

  std::cout << "END _extractBitplaneNumcode" << std::endl;
}

void ChannelExtractor::_extractBitplaneSyndrome(const BitplaneExtractor &bp) {

  ICOV_TRACE << "CH " << std::to_string(m_params.CompID)
             << ": _extractBitplaneSyndrome";
  ICOV_TRACE << bp.ReadSyndromeByteSize << "/" << bp.StoredSyndromeByteSize
             << " bytes to read";

  ICOV_TRACE
      << "SYNDROME POS IN ENCODED FILE START FROM "
      << m_icovFile->getStream(icov::bitstream::eIcovStream::SYNDROME).tellg();

  ICOV_TRACE << "BITSTREAM POS BEFORE READ " << m_bitstream->getWritePosition();

  int total_read = 0;

  while (total_read < bp.ReadSyndromeByteSize) {
    int read = std::min((int)(bp.ReadSyndromeByteSize - total_read),
                        (int)((m_bitstream->getBufferSize() -
                               m_bitstream->getWritePosition())));

    m_icovFile->readSyndrome(m_bitstream->writableData(), read, read);
    m_bitstream->shiftWritePosition(read);

    total_read += read;
  }

  icov::assert::equal(total_read, (int)bp.ReadSyndromeByteSize,
                      "Syndrome read size bad value");
  int offset = bp.StoredSyndromeByteSize - bp.ReadSyndromeByteSize;
  icov::assert::check_not(offset < 0, "Offset can't be negative");
  m_icovFile->offsetSyndrome(offset);

  ICOV_TRACE
      << "SYNDROME POS NOW IS "
      << m_icovFile->getStream(icov::bitstream::eIcovStream::SYNDROME).tellg();

  ICOV_TRACE << "BITSTREAM POS AFTER READ " << m_bitstream->getWritePosition();

  std::cout << "END _extractBitplaneSyndrome" << std::endl;
}

int ChannelExtractor::getCoderSize() const { return m_ldpc_coder->get_n(); }
