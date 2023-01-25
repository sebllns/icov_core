#include "OutputMedia.h"
#include "helpers.h"
#include "icov_stream.h"
#include "typedef.h"
#include "video_layer/encoded_stream.h"
#include <boost/serialization/array_wrapper.hpp>
#include <limits>
#include <memory>
#include <string>
namespace icov {
namespace encoder {

OutputIcov::OutputIcov(const boost::filesystem::path &path)
    : icov::IOutputIcov(path), last_bit_shift(0), last_byte(0) {
  m_outputStream = std::make_unique<std::ofstream>(path, std::ios_base::binary);

  for (int i = eIcovStream::MEDIA_HEADER; i < eIcovStream::COUNT; ++i) {
    std::string part_file = path.string() + "." + std::to_string(i);
    m_fileParts[i] =
        std::make_unique<std::ofstream>(part_file, std::ios_base::binary);
  }

  rates_count = 0;
  total_lost = 0;
  total_shift = 0;
  total_write_bits = 0;
  m_total_write_byte_block[0] = 0;
  m_total_write_byte_block[1] = 0;
  m_total_write_byte_frame[0] = 0;
  m_total_write_byte_frame[1] = 0;
}

const icov::ContentParameterSet &OutputIcov::contentParameterSet() const {
  return m_contentParameterSet;
}

icov::ContentParameterSet &OutputIcov::contentParameterSet() {
  return m_contentParameterSet;
}

void OutputIcov::writeQaryParams(const BitSymbolProvider::Pz &bsp) {
  if (bsp.lx == 0) {
    std::cout << "SI IDENTICAL" << std::endl;
    icov::helpers::io::write_value<icov::byte>(
        getStream(eIcovStream::ENCODE_PARAMS), 0, true);
    m_total_write_byte_block[0] += sizeof(icov::byte);
  } else {
    icov::bitstream::writeQaryParams(m_qaryParams, bsp.q_idx, bsp.zmin,
                                     bsp.zmax);
    icov::helpers::io::write_array(getStream(eIcovStream::ENCODE_PARAMS),
                                   m_qaryParams, QARY_PARAMS_SIZE, true);
    m_total_write_byte_block[0] += QARY_PARAMS_SIZE * sizeof(icov::byte);
  }
}

void OutputIcov::writeContentParameterSet() {
  boost::archive::binary_oarchive o_archive(
      *m_fileParts[eIcovStream::MEDIA_HEADER], boost::archive::no_header);
  o_archive &m_contentParameterSet;
}

void OutputIcov::writeSyndromeFlag(bool flag) {
  if (flag) {
    icov::helpers::bitstream::setBit(BYTE_SIZE - last_bit_shift, last_byte);
  }
  last_bit_shift = (last_bit_shift + 1) % BYTE_SIZE;
  if (last_bit_shift == 0) {
    icov::helpers::io::write_value(getStream(eIcovStream::SYNDROME), last_byte,
                                   true);
    last_byte = 0;
  }
}

void OutputIcov::writeSyndrome(const Syndrome &s) {
  const int byte_size = bitToByteCount(s.size() + last_bit_shift);
  icov::byte darr[byte_size];

  std::fill(darr, darr + byte_size, 0);

  darr[0] = last_byte;
  last_byte = 0;

  last_bit_shift = write_bits(s, darr, last_bit_shift);

  if (last_bit_shift != 0)
    last_byte = darr[byte_size - 1];

  int lost = byteToBitCount(byte_size) - s.size();

  total_lost += lost;
  total_write_bits += s.size();
  total_shift += last_bit_shift;

  m_total_write_byte_block[1] += byte_size;

  std::cout << "WRITE BYTES " << byte_size << std::endl;
  std::cout << "WRITE BITS " << s.size() << std::endl;
  std::cout << "SHIFT BITS " << last_bit_shift << std::endl;
  std::cout << "LOST BITS " << lost << std::endl;

  std::cout << "TOTAL WRITE BITS " << total_write_bits << std::endl;
  std::cout << "TOTAL SHIFT BITS " << total_shift << std::endl;
  std::cout << "TOTAL LOST BITS " << total_lost << std::endl;

  //  boost::archive::binary_oarchive
  //  o_archive(*m_fileParts[eIcovStream::SYNDROME],
  //                                            boost::archive::no_header);
  //  o_archive &boost::serialization::make_array(darr, byte_size);

  //  m_fileParts[eIcovStream::SYNDROME]->write((char *)darr, sizeof(darr));
  //  m_fileParts[eIcovStream::SYNDROME]->flush();

  helpers::io::write_array(*m_fileParts[eIcovStream::SYNDROME], darr,
                           byte_size - (last_bit_shift != 0 ? 1 : 0), true);
}

void OutputIcov::writeFrameSize() {
  helpers::io::write_value<uint32_t>(*m_fileParts[eIcovStream::FRAMES_SIZE],
                                     m_total_write_byte_frame[0], false);
  helpers::io::write_value<uint32_t>(*m_fileParts[eIcovStream::FRAMES_SIZE],
                                     m_total_write_byte_frame[1], true);
  m_total_write_byte_frame[0] = 0;
  m_total_write_byte_frame[1] = 0;
}

void OutputIcov::writeBlockSize() {

  std::cout << "WRITE SIZE " << m_total_write_byte_block[0] << " "
            << m_total_write_byte_block[1] << std::endl;

  helpers::io::write_value<uint32_t>(*m_fileParts[eIcovStream::BLOCKS_SIZE],
                                     m_total_write_byte_block[0], true);
  helpers::io::write_value<uint32_t>(*m_fileParts[eIcovStream::BLOCKS_SIZE],
                                     m_total_write_byte_block[1], true);

  m_total_write_byte_frame[0] += m_total_write_byte_block[0];
  m_total_write_byte_frame[1] += m_total_write_byte_block[1];

  m_total_write_byte_block[0] = 0;
  m_total_write_byte_block[1] = 0;
}

void OutputIcov::writeSIRates(const NumcodeIdxVec &v) {
  rates_count += v.size() * sizeof(rate_t);
  // CHECKED RATES
  std::cout << "WRITE RATES";
  for (auto r : v) {
    writeSIRate(r);
  }
  std::cout << std::endl;
}

void OutputIcov::writeSIRate(const NumcodeIdx &r) {
#ifdef ICOV_ASSERTION_ENABLED
  icov::helpers::assert::check(
      r < std::numeric_limits<rate_t>::max(),
      "Value " + std::to_string(r) + " overflows the max possible value " +
          std::to_string(std::numeric_limits<rate_t>::max()));
#endif

  helpers::io::write_value<rate_t>(*m_fileParts[eIcovStream::ENCODE_PARAMS],
                                   static_cast<rate_t>(r), true);

  std::cout << " " << r;
  m_total_write_byte_block[0] += sizeof(rate_t);
}

void OutputIcov::writeSIMode(const uint &v) {
  helpers::io::write_value<icov::byte>(*m_fileParts[eIcovStream::ENCODE_PARAMS],
                                       static_cast<rate_t>(v), true);
}

void OutputIcov::flush(int id) {
  if (id == eIcovStream::COUNT) {
    for (const auto &p : m_fileParts)
      p.second->flush();
  } else {
    m_fileParts[id]->flush();
  }
}

std::ostream &OutputIcov::getStream(int id) { return *m_fileParts[id]; }
} // namespace encoder
} // namespace icov
