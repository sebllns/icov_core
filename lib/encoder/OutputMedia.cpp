#include "OutputMedia.h"
#include "helpers.h"
#include "typedef.h"
#include "video_layer/encoded_stream.h"
#include <boost/serialization/array_wrapper.hpp>
#include <limits>
#include <memory>
#include <string>
namespace icov {
namespace encoder {

OutputIcov::OutputIcov(const boost::filesystem::path &path)
    : icov::IOutputIcov(path) {
  m_outputStream = std::make_unique<std::ofstream>(path, std::ios_base::binary);

  for (int i = eIcovStream::MEDIA_HEADER; i < eIcovStream::COUNT; ++i) {
    std::string part_file = path.string() + "." + std::to_string(i);
    m_fileParts[i] =
        std::make_unique<std::ofstream>(part_file, std::ios_base::binary);
    *m_fileParts[i] << std::ios_base::unitbuf;
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

void OutputIcov::writeContentParameterSet() {
  boost::archive::binary_oarchive o_archive(
      *m_fileParts[eIcovStream::MEDIA_HEADER], boost::archive::no_header);
  o_archive &m_contentParameterSet;
}

void OutputIcov::writeSyndrome(bool nocoding, const Syndrome &s) {
  const int byte_size = bitToByteCount(s.size() + 1);
  icov::byte darr[byte_size];
  std::fill(darr, darr + byte_size, 0);
  if (nocoding)
    icov::helpers::bitstream::setBit(BYTE_SIZE,
                                     darr[0]); /*set bit for no coding*/
  int shift = write_bits(darr, s);
  int lost = byteToBitCount(byte_size) - s.size();

  total_lost += lost;
  total_write_bits += s.size();
  total_shift += shift;

  m_total_write_byte_block[1] += byte_size;

  std::cout << "WRITE BYTES " << byte_size << std::endl;
  std::cout << "WRITE BITS " << s.size() << std::endl;
  std::cout << "SHIFT BITS " << shift << std::endl;
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

  helpers::io::write_value(*m_fileParts[eIcovStream::SYNDROME], darr, true);
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

  helpers::io::write_value<rate_t>(*m_fileParts[eIcovStream::RATES],
                                   static_cast<rate_t>(r), true);

  std::cout << " " << r;
  m_total_write_byte_block[0] += sizeof(rate_t);
}
} // namespace encoder
} // namespace icov
