#include "OutputIcovBase.h"
#include "version.h"
#include "video_layer/ContentParameterSet.h"
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
    : icov::IOutputIcov(path) {

  (*m_outputStream) << "ICOV " << ICOV_CORE_VERSION << std::endl;
  int flags = std::ios_base::out | openflag;
  for (int i = bitstream::eIcovStream::MEDIA_HEADER;
       i < bitstream::eIcovStream::COUNT; ++i) {
    std::string part_file = path.string() + "." + std::to_string(i);
    m_fileParts[i] = std::make_unique<std::ofstream>(
        part_file, static_cast<std::ios_base::openmode>(flags));
  }
  m_total_write_byte.fill(0);
}

const bitstream::ContentParameterSet &
OutputIcovBase::contentParameterSet() const {
  return m_contentParameterSet;
}

bitstream::ContentParameterSet &OutputIcovBase::contentParameterSet() {
  return m_contentParameterSet;
}

void OutputIcovBase::flush(int id) {
  if (id == bitstream::eIcovStream::COUNT) {
    for (const auto &p : m_fileParts)
      p.second->flush();
  } else {
    m_fileParts[id]->flush();
  }
}

std::ostream &OutputIcovBase::getStream(int id) { return *m_fileParts[id]; }

int OutputIcovBase::channel() const
{
  return m_channel;
}

void OutputIcovBase::setChannel(int newChannel)
{
  m_channel = newChannel;
}
} // namespace encoder
} // namespace icov
