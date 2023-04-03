#include "ChannelExtractor.h"

#include "helpers.h"
#include "ldpc_codec_layer/factory/CoderFactory.h"
#include "omnidirectional_layer/BlockPrediction.h"
#include "typedef.h"
#include "video_layer/encoded_stream.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"
#include <algorithm>
#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <sys/types.h>

namespace fs = boost::filesystem;
using namespace icov::extractor;
using SI_SIDE = BlockPrediction::predType;

bitstream::byte ChannelExtractor::SyndromePadding = 0;

ChannelExtractor::ChannelExtractor(/*const params &p,*/
                                   std::shared_ptr<InputIcov> &icovFile,
                                   const bitstream::sp_IcovExtractedBitstream
                                       &bitstream,
                                   const LdpcCoderParameters &LDPC_params)
    : /*m_params(p),*/ m_ldpc_coder(
          icov::LdpcCoderFactory::create_coder(LDPC_params)),
      m_icovFile(icovFile), m_bitstream(bitstream),
      m_maxSyndromeBufferLength(
          bitstream::bitToByteCount(m_ldpc_coder->get_m_max()))
/*,m_readingBuffer(m_maxSyndromeBufferLength)*/ {
  m_bitplanes.reserve(MAX_BITPLANE_COUNT);
  //  m_extractData.BitplaneNumcodes.reset(MAX_BITPLANE_COUNT);

  // m_extractData.SyndromeBuf.reset(m_maxSyndromeBufferLength);
}

ChannelExtractor::~ChannelExtractor() {}

void ChannelExtractor::extract_block(
    const Block &b, BlockPrediction::predType block_prediction) {
  ICOV_ASSERT_MSG(block_prediction != BlockPrediction::predType::UNKNOWN,
                  "Prediction unknown");

  bool is_access_block = (block_prediction == BlockPrediction::predType::NONE);
  bool is_inter_prediction =
      (block_prediction == BlockPrediction::predType::INTER);
  bool is_intra_prediction = !is_access_block && !is_inter_prediction;

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
    bitstream::byte dirmode = 0;
    m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS, dirmode);
    ICOV_ASSERT_MSG(dirmode > 0, "No direction mode for the prediction");
    m_bitstream->push(dirmode);
    bitstream::byte pred_hdr = 0;
    m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS, pred_hdr);
    bitstream::byte zmin = 0, zmax = 0;
    if (pred_hdr != 0) {
      m_icovFile->read(bitstream::eIcovStream::ENCODE_PARAMS, zmin);
      m_bitstream->push(zmin);
      m_icovFile->read(bitstream::eIcovStream::ENCODE_PARAMS, zmax);
      m_bitstream->push(zmax);
    }
    for (auto &bp : m_bitplanes) {
      m_icovFile->read(bitstream::eIcovStream::ENCODE_PARAMS,
                       bp.ReadSyndromeQuantizedSize);

      bp.ReadSyndromeBitSize = bitstream::inverseQuantizedSize(
          bp.ReadSyndromeQuantizedSize, *m_ldpc_coder);

      bitstream::getByteCount(bp.StoredSyndromeBitSize,
                              bp.StoredSyndromeByteSize,
                              bp.ReadSyndromePadding);
    }
  } else if (is_inter_prediction) {
    // TODO IMPLEMENT
  }

  for (auto &bp : m_bitplanes) {
    extract_bitplane_syndrome(bp);
  }
}

void ChannelExtractor::extract_syndrome_sizes(bool is_access_block) {
  // read bitplane count
  m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS,
                   m_bitplaneCount);
  // output.resize(bitplaneCount);
  std::cout << " m_bitplaneCount " << (int)m_bitplaneCount << "\n";

  m_bitplanes.resize(m_bitplaneCount);

  // for each bitplane read stored syndrome size
  for (auto &bp : m_bitplanes) {
    bp.StoredSyndromeQuantizedSize = 0;
    bp.StoredSyndromeBitSize = 0;
    bp.StoredSyndromeByteSize = 0;

    bp.ReadSyndromeQuantizedSize = 0;
    bp.ReadSyndromeBitSize = 0;
    bp.ReadSyndromeByteSize = 0;
    bp.ReadSyndromePadding = 0;

    if (is_access_block) {
      bp.StoredSyndromeQuantizedSize = 1;
    } else {
      m_icovFile->read(icov::bitstream::eIcovStream::ENCODE_PARAMS,
                       bp.StoredSyndromeQuantizedSize);
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
}

void ChannelExtractor::pass_access_block() {
  ushort syndrome_size = 0, syndrome_padding = 0;
  bitstream::getByteCount(m_ldpc_coder->get_n() * m_bitplaneCount -
                              SyndromePadding,
                          syndrome_size, syndrome_padding);
  m_icovFile->offsetSyndrome(syndrome_size);
  SyndromePadding =
      ((syndrome_padding == 0) ? (0) : (BYTE_SIZE - syndrome_padding));
}

void ChannelExtractor::pass_prediction() {
  m_icovFile->passPrediction(m_bitplaneCount);
}

void ChannelExtractor::resetSyndromePadding() { SyndromePadding = 0; }

int ChannelExtractor::getSyndromePadding() { return SyndromePadding; }

void ChannelExtractor::extract_bitplane_syndrome(const BitplaneExtractor &bp) {
  m_icovFile->readSyndrome(m_bitstream->writableData(), bp.ReadSyndromeByteSize,
                           bp.StoredSyndromeByteSize);
  m_bitstream->shiftWritePosition(bp.ReadSyndromeByteSize);
}

int ChannelExtractor::get_coder_size() const { return m_ldpc_coder->get_n(); }
