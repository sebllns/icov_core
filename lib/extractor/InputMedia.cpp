#include "InputMedia.h"

#include "Media.h"
#include "icov_assert.h"
#include "icov_log.h"
#include "icov_path.h"
#include "video_layer/ContentParameterSet.h"
#include "video_layer/encoded_stream.h"
#include "video_layer/extracted_stream.h"
#include "video_layer/icov_stream.h"

#include <boost/filesystem/path.hpp>
#include <boost/serialization/array_wrapper.hpp>

#include <cstdint>
#include <fstream>
#include <ios>
#include <iterator>
#include <limits>
#include <memory>
#include <string>

icov::extractor::InputIcov::InputIcov(const boost::filesystem::path &path)
    : icov::IInputIcov<pstream>(path), m_blockCount(0) {
  for (int i = bitstream::eIcovStream::MEDIA_HEADER;
       i < bitstream::eIcovStream::COUNT; ++i) {
    fs::path part_file = path / bitstream::getIcovPartName(i);
    path::assert_file_exists(part_file.string());
    ICOV_TRACE << "Open file part " << part_file << std::endl;
    m_fileParts[i] = std::make_unique<std::ifstream>(
        part_file, std::ios_base::binary | std::ios_base::in);
  }
}

icov::extractor::InputIcov::~InputIcov() {}

const icov::bitstream::ContentParameterSet &
icov::extractor::InputIcov::contentParameterSet() const {
  return m_contentParameterSet;
}

icov::bitstream::ContentParameterSet &
icov::extractor::InputIcov::contentParameterSet() {
  return m_contentParameterSet;
}

void icov::extractor::InputIcov::readContentParameterSet() {
  boost::archive::binary_iarchive archive(
      *m_fileParts[bitstream::eIcovStream::MEDIA_HEADER],
      boost::archive::no_header);
  archive &m_contentParameterSet;
  m_frameCount = m_contentParameterSet.content_framecount;

  if (m_contentParameterSet.icov_version != ICOV_CORE_VERSION) {
    ICOV_WARNING << "Decoder version " << ICOV_CORE_VERSION
                 << " differs from encoder version "
                 << m_contentParameterSet.icov_version;
  }
}

void icov::extractor::InputIcov::offsetSyndrome(uint offset) {
  if (offset != 0)
    getStream(bitstream::eIcovStream::SYNDROME)
        .seekg(offset, std::ios_base::cur);
}

void icov::extractor::InputIcov::readSyndrome(icov::byte *output,
                                              uint64_t read_size,
                                              uint64_t stored_size) {
  auto &str = getStream(icov::bitstream::eIcovStream::SYNDROME);
  if (read_size > 0) {
    icov::io::read_value(str, output, read_size);
  }

  if (stored_size > read_size) {
    offsetSyndrome(stored_size - read_size);
  }
}

void icov::extractor::InputIcov::readBlockPos() {
  m_blockCount = m_contentParameterSet.icu_count;
  m_rates_pos.resize(m_blockCount + 1, 0);
  m_synd_pos.resize(m_blockCount + 1, 0);

  std::streampos rate_pos = m_rates_pos[0];
  std::streampos synd_pos = m_synd_pos[0];

  for (size_t i = 1; i < m_rates_pos.size(); ++i) {
    uint32_t rate_size = 0;
    uint32_t synd_size = 0;

    icov::io::read_value(getStream(bitstream::eIcovStream::BLOCKS_SIZE),
                         &rate_size);
    icov::io::read_value(getStream(bitstream::eIcovStream::BLOCKS_SIZE),
                         &synd_size);

    rate_pos += rate_size;
    synd_pos += synd_size;

    m_rates_pos[i] = rate_pos;
    m_synd_pos[i] = synd_pos;

    ICOV_TRACE << "BLOCK POS " << std::next(m_contentParameterSet.encoded_blocks.cbegin(), i)->first << ": " << rate_pos
               << ", " << synd_pos << std::endl;
  }
}

void icov::extractor::InputIcov::selectBlock(const omnidirectional_layer::Block::id_t &id)
{
  auto id_pos = getBlockIndexInStream(id);
  ICOV_TRACE << "select block in stream: block_id=" << id << ", id_pos=" << id_pos;
  ICOV_TRACE << "rate_pos=" << m_rates_pos[id_pos] << ", synd_pos=" << m_synd_pos[id_pos];
  getStream(bitstream::eIcovStream::ENCODE_PARAMS).clear();
  getStream(bitstream::eIcovStream::ENCODE_PARAMS)
      .seekg(m_rates_pos[id_pos], std::ios_base::beg);
  getStream(bitstream::eIcovStream::SYNDROME).clear();
  getStream(bitstream::eIcovStream::SYNDROME)
      .seekg(m_synd_pos[id_pos], std::ios_base::beg);

  getStream(bitstream::eIcovStream::METRICS).clear();
  getStream(bitstream::eIcovStream::METRICS)
      .seekg((m_frameID * m_blockCount + id_pos) * sizeof(float),
             std::ios_base::beg);
}

void icov::extractor::InputIcov::readSyndromesSizes(
    std::vector<int> &bitplaneCount, std::vector<NumcodeIdxVec> &output) {
  for (int c = 0; c < (int)output.size(); ++c) {
    // read bitplane count
    bitplaneCount[c] = readByteIntParam();
    output[c] = NumcodeIdxVec(bitplaneCount[c]);
    std::cout << "COMP " << c << " N " << output[c].size() << "\n";
    for (int bp = 0; bp < (int)output[c].size(); ++bp) {
      // for each bitplane read stored syndrome size
      output[c][bp] = readByteIntParam();
      // std::cout << " " << output[c][bp];
    }
    std::cout << std::endl;
  }
}

void icov::extractor::InputIcov::readSyndromesSizes(NumcodeIdxVec &output) {
  // read bitplane count
  int bitplaneCount = readByteIntParam();
  // output.resize(bitplaneCount);
  std::cout << " bitplaneCount " << bitplaneCount << "\n";
  for (int bp = 0; bp < bitplaneCount; ++bp) {
    // for each bitplane read stored syndrome size
    output.push_back(readByteIntParam());
    // std::cout << " " << output[c][bp];
  }
  std::cout << std::endl;
}

void icov::extractor::InputIcov::jumpPrediction(int bitplaneCount) {
  byte val = 0;
  auto &str = getStream(bitstream::eIcovStream::ENCODE_PARAMS);

  // read prediction mode + 1
  icov::io::read_value(str, &val);

  if (val > 0) {
    // read q_idx + 1
    icov::io::read_value(str, &val);
    // std::cout << static_cast<int>(val) << std::endl;
    if (val > 0) {
      // std::cout << "seek " << (val + 2) << std::endl;
      // shift qary params
      int byte_shift = (bitplaneCount + 2) * sizeof(byte);
      str.seekg(byte_shift, std::ios_base::cur);
    } // else no coding
  }   // else no prediction
}

void icov::extractor::InputIcov::selectPrediction(
    ushort pred_idx, const std::vector<int> &bitplaneCount) {

  int channelCount = bitplaneCount.size();
  for (int i = 0; i < pred_idx; ++i) {
    for (int c = 0; c < channelCount; ++c) {
      jumpPrediction(bitplaneCount[c]);
    }
  }
}

int icov::extractor::InputIcov::readByteIntParam() {
  byte val = 0;
  auto &str = getStream(bitstream::eIcovStream::ENCODE_PARAMS);
  icov::io::read_value(str, &val);
  std::cout << "(int) = " << (int)val << std::endl;
  return static_cast<int>(val);
}

void icov::extractor::InputIcov::readQaryParams(byte header,
                                                int &q_idx, int &z_min,
                                                int &z_max) {
  // q
  m_qaryParams[0] = header;
  auto &str = getStream(bitstream::eIcovStream::ENCODE_PARAMS);
  // z_min
  icov::io::read_value(str, &m_qaryParams[1]);
  // z_max
  icov::io::read_value(str, &m_qaryParams[2]);
  icov::bitstream::readQaryParams(m_qaryParams, q_idx, z_min, z_max);

  ICOV_ASSERT_MSG(z_min < z_max, "z_min >= z_max is not possible");
}

void icov::extractor::InputIcov::readSIRates(NumcodeIdxVec &v) {
  for (int i = 0; i < (int)v.size(); ++i) {
    // note : type byte should be selected depending on the rate byte
    // encoding length
    byte val = 0;
    auto &str = getStream(bitstream::eIcovStream::ENCODE_PARAMS);
    icov::io::read_value(str, &val);
    v[i] = static_cast<NumcodeIdx>(val);
  }
}

void icov::extractor::InputIcov::extractBlock(
    icov::bitstream::BlockChannelDecoderParams &output, int icu_idx) {}

void icov::extractor::InputIcov::writeInfos(std::ofstream &output, int key,
                                            void *param) {}

void icov::extractor::InputIcov::setFrame(frame_t frame) {
  uint32_t frameRateSize = 0;
  uint32_t frameSyndromeSize = 0;

  getStream(bitstream::eIcovStream::FRAMES_SIZE).clear();

  while (frame > m_frameID) {
    icov::io::read_value(getStream(bitstream::eIcovStream::FRAMES_SIZE),
                         &frameRateSize);
    icov::io::read_value(getStream(bitstream::eIcovStream::FRAMES_SIZE),
                         &frameSyndromeSize);

    m_rates_pos[0] += frameRateSize;
    m_synd_pos[0] += frameSyndromeSize;

    ++m_frameID;
  }

  while (frame < m_frameID) {

    getStream(bitstream::eIcovStream::FRAMES_SIZE)
        .seekg(-2 * (int)sizeof(frameRateSize), std::ios_base::cur);

    icov::io::read_value(getStream(bitstream::eIcovStream::FRAMES_SIZE),
                         &frameRateSize);
    icov::io::read_value(getStream(bitstream::eIcovStream::FRAMES_SIZE),
                         &frameSyndromeSize);

    m_rates_pos[0] -= frameRateSize;
    m_synd_pos[0] -= frameSyndromeSize;

    getStream(bitstream::eIcovStream::FRAMES_SIZE)
        .seekg(-2 * (int)sizeof(frameRateSize), std::ios_base::cur);

    --m_frameID;
  }

  IInputIcov<pstream>::setFrame(frame);

  getStream(bitstream::eIcovStream::BLOCKS_SIZE)
      .seekg(m_frameID * m_blockCount * 2 * sizeof(uint32_t),
             std::ios_base::beg);
  getStream(bitstream::eIcovStream::ENCODE_PARAMS)
      .seekg(m_rates_pos[0], std::ios_base::beg);
  getStream(bitstream::eIcovStream::SYNDROME).seekg(m_synd_pos[0], std::ios_base::beg);
  getStream(bitstream::eIcovStream::METRICS)
      .seekg(m_frameID * m_blockCount * sizeof(float), std::ios_base::beg);

  // std::cout << m_rates_pos[0] << ", " << m_synd_pos[0] << std::endl;

  this->readBlockPos();
}

icov::omnidirectional_layer::Block::id_t
icov::extractor::InputIcov::getBlockIndexInStream(
    const omnidirectional_layer::Block::id_t &id) const {
  if (m_contentParameterSet.icu_count == 0)
    return id - 1;

#if ICOV_ASSERTION_ENABLED
  ICOV_ASSERT_MSG(m_contentParameterSet.encoded_blocks.count(id),
                  "Block not encoded: " + std::to_string(id));
#endif
  return m_contentParameterSet.encoded_blocks.at(id);
}

void icov::extractor::InputIcov::resetStreams() {
  //  for (auto &p : m_fileParts) {
  //    p.second->clear();
  //    p.second->seekg(0, std::ios_base::beg);
  //    ICOV_DEBUG << "AFTER CLEAR READ POS " << p.second->tellg();
  //  }

  // m_frameID = 0;

  getStream(bitstream::eIcovStream::ENCODE_PARAMS).clear();
  getStream(bitstream::eIcovStream::SYNDROME).clear();

  getStream(bitstream::eIcovStream::ENCODE_PARAMS).seekg(0, std::ios_base::beg);
  getStream(bitstream::eIcovStream::SYNDROME).seekg(0, std::ios_base::beg);
}

std::istream &icov::extractor::InputIcov::getStream(int id) {
  return *m_fileParts[id];
}
