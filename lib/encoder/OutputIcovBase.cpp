#include "OutputIcovBase.h"
#include "typedef.h"
#include "video_layer/encoded_stream.h"
#include <algorithm>
#include <boost/serialization/array_wrapper.hpp>
#include <limits>
#include <memory>
#include <string>
namespace icov {
namespace encoder {

OutputIcovBase::OutputIcovBase(const boost::filesystem::path &path,
                               int openflag)
    : icov::IOutputIcov(path), last_bit_shift(0), last_byte(0) {

  int flags = std::ios_base::out | openflag;
  for (int i = eIcovStream::MEDIA_HEADER; i < eIcovStream::COUNT; ++i) {
    std::string part_file = path.string() + "." + std::to_string(i);
    m_fileParts[i] = std::make_unique<std::ofstream>(
        part_file, static_cast<std::ios_base::openmode>(flags));
  }

  total_lost = 0;
  total_shift = 0;
  total_write_bits = 0;

  m_total_write_byte_block[0] = 0;
  m_total_write_byte_block[1] = 0;
  m_total_write_byte_frame[0] = 0;
  m_total_write_byte_frame[1] = 0;
}

const icov::ContentParameterSet &OutputIcovBase::contentParameterSet() const {
  return m_contentParameterSet;
}

icov::ContentParameterSet &OutputIcovBase::contentParameterSet() {
  return m_contentParameterSet;
}

void OutputIcovBase::flush(int id) {
  if (id == eIcovStream::COUNT) {
    for (const auto &p : m_fileParts)
      p.second->flush();
  } else {
    m_fileParts[id]->flush();
  }
}

std::ostream &OutputIcovBase::getStream(int id) { return *m_fileParts[id]; }
} // namespace encoder
} // namespace icov
