#include "OutputIcovBase.h"

#include "EncoderStatistics.h"
#include "video_layer/ContentParameterSet.h"

#include <boost/filesystem/path.hpp>

#include <memory>
#include <string>

namespace icov {
namespace encoder {

OutputIcovBase::OutputIcovBase(const boost::filesystem::path &path)
    : icov::IOutputIcov(path), m_channel(0) {}

const bitstream::ContentParameterSet &
OutputIcovBase::contentParameterSet() const {
  return m_contentParameterSet;
}

bitstream::ContentParameterSet &OutputIcovBase::contentParameterSet() {
  return m_contentParameterSet;
}

void OutputIcovBase::setInfo(const std::string &key, const std::string &value) {
  m_info[key] = value;
}

void OutputIcovBase::setStatistics(const EncoderStatistics &stats) {
  m_stats = std::make_unique<EncoderStatistics>(stats);
}

std::ostream &OutputIcovBase::debugStream() {
  if (!m_debug) {
    fs::path p = getPath();
    m_debug = std::make_unique<std::ofstream>(p.replace_extension(".debug.txt"));
  }
  return *m_debug;
}

int OutputIcovBase::channel() const { return m_channel; }

void OutputIcovBase::setChannel(int newChannel) { m_channel = newChannel; }

fs::path OutputIcovBase::statsPath() const {
  fs::path p = getPath();
  return p.replace_extension(".stats.json");
}

fs::path OutputIcovBase::referenceDir() const {
  fs::path p = getPath();
  return p.replace_extension(".ref");
}

} // namespace encoder
} // namespace icov
