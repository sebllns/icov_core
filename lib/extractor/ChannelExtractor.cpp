#include "ChannelExtractor.h"

#include "FastArray.h"

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

  std::cout << "BP ";
  m_bitstream->push(m_bitplaneCount);

  if (is_access_block) {
    ushort syndrome_size = 0, syndrome_padding = 0;
    //    bitstream::getByteCount(m_ldpc_coder->get_n() * m_bitplaneCount -
    //                                SyndromePadding,
    //                            syndrome_size, syndrome_padding);

    //    m_icovFile->readSyndrome(m_readingBuffer, syndrome_size,
    //    syndrome_size); SyndromePadding =
    //        ((syndrome_padding == 0) ? (0) : (BYTE_SIZE - syndrome_padding));

  } else if (is_intra_prediction) {
    _extractIntraParams();
  } else if (is_inter_prediction) {
    // TODO IMPLEMENT
  }

  for (auto &bp : m_bitplanes) {
    _extractBitplaneSyndrome(bp);
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
  std::cout << std::to_string(m_params.CompID) << ": _extractIntraParams "
            << std::endl;

  bitstream::byte dirmode = 0;
  m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS, dirmode);

  bitstream::assertPredictionAvailable(dirmode);

  bitstream::VideoStreamDebugger::get_const_instance().assertDirModeEqual(
      dirmode);

  std::cout << "DM ";
  m_bitstream->push(dirmode);
  bitstream::byte pred_hdr = bitstream::flag_t::NoCodingSI;
  m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS, pred_hdr);
  std::cout << "QZ ";

  m_bitstream->push(pred_hdr);

  bool codingFlag = bitstream::getCodingFlag(pred_hdr);
  bitstream::VideoStreamDebugger::get_const_instance().assertcodingSIEqual(
      codingFlag);

  if (codingFlag) {
    bitstream::byte zmin = 0, zmax = 0;

    bitstream::byte q0 = bitstream::getQaryQParam(pred_hdr);
    bitstream::VideoStreamDebugger::get_const_instance().assertQaryParamEqual(
        ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Q, q0);

    m_icovFile->read(bitstream::eIcovStream::ENCODE_PARAMS, zmin);
    bitstream::VideoStreamDebugger::get_const_instance().assertQaryParamEqual(
        ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Z_MIN, zmin);
    m_bitstream->push(zmin);

    m_icovFile->read(bitstream::eIcovStream::ENCODE_PARAMS, zmax);
    bitstream::VideoStreamDebugger::get_const_instance().assertQaryParamEqual(
        ldpc_codec_layer::QaryDistribution::QaryParamsIndex_Z_MAX, zmax);
    m_bitstream->push(zmax);

    for (auto &bp : m_bitplanes) {
      _extractBitplaneNumcode(bp);
    }
  }
  std::cout << "END _extractIntraParams" << std::endl;
}

void ChannelExtractor::_extractBitplaneNumcode(BitplaneExtractor &bp) {
  std::cout << std::to_string(m_params.CompID) << ": _extractBitplaneNumcode"
            << std::endl;

  m_icovFile->read(bitstream::eIcovStream::ENCODE_PARAMS,
                   bp.ReadSyndromeQuantizedSize);

  bitstream::VideoStreamDebugger::get_const_instance().assertNumcodeEqual(
      bp.Index, (NumcodeIdx)bp.ReadSyndromeQuantizedSize);

  m_bitstream->push(bp.ReadSyndromeQuantizedSize);

  bp.ReadSyndromeBitSize = bitstream::inverseQuantizedSize(
      bp.ReadSyndromeQuantizedSize, *m_ldpc_coder);

  bitstream::getByteCount(bp.StoredSyndromeBitSize, bp.StoredSyndromeByteSize,
                          bp.ReadSyndromePadding);

  std::cout << "END _extractBitplaneNumcode" << std::endl;
}

void ChannelExtractor::_extractBitplaneSyndrome(const BitplaneExtractor &bp) {

  std::cout << std::to_string(m_params.CompID) << ": _extractBitplaneSyndrome"
            << std::endl;

  m_icovFile->readSyndrome(m_bitstream->writableData(), bp.ReadSyndromeByteSize,
                           bp.StoredSyndromeByteSize);
  m_bitstream->shiftWritePosition(bp.ReadSyndromeByteSize);

  std::cout << "END _extractBitplaneSyndrome" << std::endl;
}

int ChannelExtractor::getCoderSize() const { return m_ldpc_coder->get_n(); }
