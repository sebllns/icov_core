#include "OutputIcovBase.h"

#include "icov_path.h"
#include "version.h"
#include "video_layer/ContentParameterSet.h"
#include "video_layer/encoded_stream.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/serialization/array_wrapper.hpp>

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
namespace icov {
namespace encoder {

OutputIcovBase::OutputIcovBase(const boost::filesystem::path &path,
                               int openflag)
    : icov::IOutputIcov(path) {

  (*m_outputStream)
      << "NOTE: This file contains no data used by the decoder, only "
         "human-readable information about the encoded ICOV file.\n"
      << "Encoder version: " << ICOV_CORE_VERSION << std::endl;
  int flags = std::ios_base::out | openflag;

  for (int i = bitstream::eIcovStream::MEDIA_HEADER;
       i < bitstream::eIcovStream::COUNT; ++i) {
    fs::path part_file = path.parent_path() / bitstream::getIcovPartName(i);
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
