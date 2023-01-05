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
namespace icov {
namespace extractor {

InputIcov::InputIcov(const boost::filesystem::path &path, size_t block_count)
    : icov::IInputIcov<pstream>(path), m_blockCount(block_count),
      m_rates_pos(block_count + 1), m_synd_pos(block_count + 1) {

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

  rates_count = 0;
  total_lost = 0;
  total_shift = 0;
  total_read_bits = 0;
  m_total_read_byte_block[0] = 0;
  m_total_read_byte_block[1] = 0;
  m_total_read_byte_frame[0] = 0;
  m_total_read_byte_frame[1] = 0;

  m_rates_pos[0] = 0;
  m_synd_pos[0] = 0;
}

const icov::ContentParameterSet &InputIcov::contentParameterSet() const {
  return m_contentParameterSet;
}

icov::ContentParameterSet &InputIcov::contentParameterSet() {
  return m_contentParameterSet;
}

void InputIcov::readContentParameterSet() {
  boost::archive::binary_iarchive archive(
      *m_fileParts[eIcovStream::MEDIA_HEADER], boost::archive::no_header);
  archive &m_contentParameterSet;
}

void InputIcov::readSyndrome(icov::byte *output, uint64_t size) {
  icov::helpers::io::read_value(getStream(eIcovStream::SYNDROME), output, size);
}

void InputIcov::readBlockPos() {
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

void InputIcov::selectBlockPrediction(uint64_t block, int pred_idx,
                                      bool access_block) {
  icov::byte val = 0;
  auto &str = getStream(eIcovStream::RATES);
  str.seekg(m_rates_pos[block], std::ios_base::beg);

  if (access_block && pred_idx > 0) {
    str.seekg(m_channelCount * sizeof(icov::byte));
    pred_idx--;
  }

  for (int i = 0; i < pred_idx * m_channelCount; ++i) {
    icov::helpers::io::read_value(str, &val);
    // std::cout << static_cast<int>(val) << std::endl;
    val = val >> 4;
    if (val > 0) {
      // std::cout << "seek " << (val + 2) << std::endl;
      str.seekg((val + 2) * sizeof(icov::byte), std::ios_base::cur);
    }
  }
}

int InputIcov::readPredictionHeader() {
  icov::byte val = 0;
  auto &str = getStream(eIcovStream::RATES);
  icov::helpers::io::read_value(str, &val);
  return static_cast<int>(val);
}

void InputIcov::readQaryParams(icov::byte header, int &lx, int &q_idx,
                               int &z_min, int &z_max) {
  m_qaryParams[0] = header;
  auto &str = getStream(eIcovStream::RATES);
  icov::helpers::io::read_value(str, &m_qaryParams[1]);
  icov::helpers::io::read_value(str, &m_qaryParams[2]);
  icov::bitstream::readQaryParams(m_qaryParams, q_idx, z_min, z_max, lx);
}

void InputIcov::readSIRates(NumcodeIdxVec &v) {
  for (int i = 0; i < (int)v.size(); ++i) {
    icov::byte val = 0;
    auto &str = getStream(eIcovStream::RATES);
    icov::helpers::io::read_value(str, &val);
    v[i] = static_cast<NumcodeIdx>(val);
  }
}

void InputIcov::readSIRate(const NumcodeIdx &r) {}
} // namespace extractor
} // namespace icov

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
  getStream(eIcovStream::RATES).seekg(m_rates_pos[0], std::ios_base::beg);
  getStream(eIcovStream::SYNDROME).seekg(m_synd_pos[0], std::ios_base::beg);
}

std::istream &icov::extractor::InputIcov::getStream(int id) {
  return *m_fileParts[id];
}
