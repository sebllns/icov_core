#include "OutputMedia.h"
#include "typedef.h"
#include "video_layer/encoded_stream.h"
#include <boost/serialization/array_wrapper.hpp>
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
  total_write_byte_block = 0;
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

void OutputIcov::writeSyndrome(const Syndrome &s) {
  const int byte_size = bitToByteCount(s.size());
  icov::byte darr[byte_size];
  std::fill(darr, darr + byte_size, 0);
  int shift = write_bits(darr, s);
  int lost = byteToBitCount(byte_size) - s.size();

  total_lost += lost;
  total_write_bits += s.size();
  total_shift += shift;

  total_write_byte_block += byte_size;

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

  m_fileParts[eIcovStream::SYNDROME]->write((char *)darr, sizeof(darr));
  m_fileParts[eIcovStream::SYNDROME]->flush();
}

void OutputIcov::writeFrameSize() {
  boost::archive::binary_oarchive o_archive(
      *m_fileParts[eIcovStream::FRAMES_SIZE], boost::archive::no_header);
  o_archive &total_write_byte_frame;
  total_write_byte_frame = 0;
}

void OutputIcov::writeBlockSize() {}

void OutputIcov::writeRates(const NumcodeIdxVec &v) {
  rates_count += v.size() * sizeof(rate_t);
  for (auto r : v)
    helpers::io::write_value<rate_t>(*m_fileParts[eIcovStream::RATES],
                                     static_cast<rate_t>(r), true);
}

} // namespace encoder
} // namespace icov
