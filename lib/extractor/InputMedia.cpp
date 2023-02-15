#include "InputMedia.h"
#include "BlockPrediction.h"
#include "Media.h"
#include "common_types.h"
#include "helpers.h"
#include "icov_stream.h"
#include "typedef.h"
#include "video_layer/encoded_stream.h"
#include <boost/serialization/array_wrapper.hpp>
#include <cstdint>
#include <fstream>
#include <ios>
#include <limits>
#include <memory>
#include <string>

icov::extractor::InputIcov::InputIcov(const boost::filesystem::path &path)
    : icov::IInputIcov<pstream>(path), m_blockCount(0), m_readBuf(nullptr),
      m_syndromeOffset(0) {
  for (int i = eIcovStream::MEDIA_HEADER; i < eIcovStream::COUNT; ++i) {
    std::string part_file = path.string() + "." + std::to_string(i);
    ICOV_TRACE << "Open file part " << part_file << std::endl;
    m_fileParts[i] = std::make_unique<std::ifstream>(
        part_file, std::ios_base::binary | std::ios_base::in);
  }

  //  icov::byte val = 0;
  //  getStream(eIcovStream::RATES).seekg(11, std::ios_base::beg);
  //    icov::helpers::io::read_value(getStream(eIcovStream::RATES), &val);

  //  getStream(eIcovStream::RATES)
  //      .read(reinterpret_cast<char *>(&val), sizeof(val));
  //  std::cout << val << std::endl;
  //  std::cout << static_cast<int>(val) << std::endl;
}

icov::extractor::InputIcov::~InputIcov() {
  if (m_readBuf)
    delete[] m_readBuf;
}

const icov::ContentParameterSet &
icov::extractor::InputIcov::contentParameterSet() const {
  return m_contentParameterSet;
}

icov::ContentParameterSet &icov::extractor::InputIcov::contentParameterSet() {
  return m_contentParameterSet;
}

void icov::extractor::InputIcov::readContentParameterSet() {
  boost::archive::binary_iarchive archive(
      *m_fileParts[eIcovStream::MEDIA_HEADER], boost::archive::no_header);
  archive &m_contentParameterSet;
}

void icov::extractor::InputIcov::initBuffer(size_t block_count,
                                            size_t bufferSize) {
  m_blockCount = block_count;
  m_rates_pos.resize(block_count + 1, 0);
  m_synd_pos.resize(block_count + 1, 0);
  m_readBuf = new icov::byte[bufferSize];
}

void icov::extractor::InputIcov::offsetSyndrome(uint offset) {
  getStream(eIcovStream::SYNDROME).seekg(offset, std::ios_base::cur);
}

void icov::extractor::InputIcov::readBlockPos() {
  std::streampos rate_pos = 0;
  std::streampos synd_pos = 0;

  for (size_t i = 1; i < m_rates_pos.size(); ++i) {
    uint32_t rate_size = 0;
    uint32_t synd_size = 0;

    icov::helpers::io::read_value(getStream(eIcovStream::BLOCKS_SIZE),
                                  &rate_size);
    icov::helpers::io::read_value(getStream(eIcovStream::BLOCKS_SIZE),
                                  &synd_size);

    rate_pos += rate_size;
    synd_pos += synd_size;

    m_rates_pos[i] = rate_pos;
    m_synd_pos[i] = synd_pos;

    std::cout << "BLOCK " << i << ": " << rate_pos << ", " << synd_pos
              << std::endl;
  }
}

void icov::extractor::InputIcov::selectBlock(const Block &block) {
  getStream(eIcovStream::ENCODE_PARAMS).clear();
  auto id = block.getIdSelf() - 1;
  getStream(eIcovStream::ENCODE_PARAMS)
      .seekg(m_rates_pos[id], std::ios_base::beg);
  getStream(eIcovStream::SYNDROME).clear();
  getStream(eIcovStream::SYNDROME).seekg(m_synd_pos[id], std::ios_base::beg);
  m_syndromeOffset = 0;
}

void icov::extractor::InputIcov::readSyndromesSizes(
    std::vector<NumcodeIdxVec> &output) {
  for (int c = 0; c < (int)output.size(); ++c) {
    // read bitplane count
    output[c] = NumcodeIdxVec(readByteIntParam());
    std::cout << "COMP " << c << " N " << output[c].size() << "\n";
    for (int bp = 0; bp < (int)output[c].size(); ++bp) {
      // for each bitplane read stored syndrome size
      output[c][bp] = readByteIntParam();
      // std::cout << " " << output[c][bp];
    }
    std::cout << std::endl;
  }
}

void icov::extractor::InputIcov::selectPrediction(ushort pred_idx,
                                                  int channelCount) {
  icov::byte val = 0;
  auto &str = getStream(eIcovStream::ENCODE_PARAMS);

  for (int i = 0; i < pred_idx * channelCount; ++i) {
    // read prediction mode
    icov::helpers::io::read_value(str, &val);

    // read q_idx + 1
    icov::helpers::io::read_value(str, &val);
    // std::cout << static_cast<int>(val) << std::endl;
    if (val > 0) {
      // std::cout << "seek " << (val + 2) << std::endl;
      // shift qary params
      str.seekg(((val >> 4) + 2) * sizeof(icov::byte), std::ios_base::cur);
    } // else no coding
  }
}

int icov::extractor::InputIcov::readByteIntParam() {
  icov::byte val = 0;
  auto &str = getStream(eIcovStream::ENCODE_PARAMS);
  icov::helpers::io::read_value(str, &val);
  std::cout << (int)val << std::endl;
  return static_cast<int>(val);
}

void icov::extractor::InputIcov::readQaryParams(icov::byte header, int &q_idx,
                                                int &z_min, int &z_max) {
  // q
  m_qaryParams[0] = header;
  auto &str = getStream(eIcovStream::ENCODE_PARAMS);
  // z_min
  icov::helpers::io::read_value(str, &m_qaryParams[1]);
  // z_max
  icov::helpers::io::read_value(str, &m_qaryParams[2]);
  icov::bitstream::readQaryParams(m_qaryParams, q_idx, z_min, z_max);
}

void icov::extractor::InputIcov::readSIRates(NumcodeIdxVec &v) {
  for (int i = 0; i < (int)v.size(); ++i) {
    // note : type icov::byte should be selected depending on the rate byte
    // encoding length
    icov::byte val = 0;
    auto &str = getStream(eIcovStream::ENCODE_PARAMS);
    icov::helpers::io::read_value(str, &val);
    v[i] = static_cast<NumcodeIdx>(val);
  }
}

void extractor::InputIcov::extractBlock(BlockDecoderParams &output,
                                        int icu_idx) {}

void icov::extractor::InputIcov::writeInfos(std::ofstream &output, int key,
                                            void *param) {}

void icov::extractor::InputIcov::setFrame(int frame) {

  uint32_t frameRateSize = 0;
  uint32_t frameSyndromeSize = 0;

  while (frame > m_frameID) {
    icov::helpers::io::read_value(getStream(eIcovStream::FRAMES_SIZE),
                                  &frameRateSize);
    icov::helpers::io::read_value(getStream(eIcovStream::FRAMES_SIZE),
                                  &frameSyndromeSize);

    m_rates_pos[0] += frameRateSize;
    m_synd_pos[0] += frameSyndromeSize;

    ++m_frameID;
  }

  while (frame < m_frameID) {

    getStream(eIcovStream::FRAMES_SIZE)
        .seekg(-2 * (int)sizeof(frameRateSize), std::ios_base::cur);

    icov::helpers::io::read_value(getStream(eIcovStream::FRAMES_SIZE),
                                  &frameRateSize);
    icov::helpers::io::read_value(getStream(eIcovStream::FRAMES_SIZE),
                                  &frameSyndromeSize);

    m_rates_pos[0] -= frameRateSize;
    m_synd_pos[0] -= frameSyndromeSize;

    getStream(eIcovStream::FRAMES_SIZE)
        .seekg(-2 * (int)sizeof(frameRateSize), std::ios_base::cur);

    --m_frameID;
  }

  IInputIcov<pstream>::setFrame(frame);

  getStream(eIcovStream::BLOCKS_SIZE)
      .seekg(m_frameID * m_blockCount, std::ios_base::beg);
  getStream(eIcovStream::ENCODE_PARAMS)
      .seekg(m_rates_pos[0], std::ios_base::beg);
  getStream(eIcovStream::SYNDROME).seekg(m_synd_pos[0], std::ios_base::beg);
}

std::istream &icov::extractor::InputIcov::getStream(int id) {
  return *m_fileParts[id];
}
