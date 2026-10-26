#include "EncoderStatistics.h"

#include <boost/algorithm/algorithm.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/file_status.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/format.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace icov {
namespace encoder {

EncoderStatistics::EncoderStatistics()
    : total_frames(0), total_block(0), total_access_block(0),
      total_bits_syndrome(0), temporal_not_used(0), temporal_enabled(0) {}

void EncoderStatistics::write(std::ostream &output) const {
  output << "Total number of encoded frames\t" << total_frames << std::endl;
  output << "Total number of encoded blocks\t" << total_block << std::endl;
  output << "Total number of encoded access blocks\t" << total_access_block
         << std::endl;

  output << "\nDistribution of the encoded rates:" << std::endl;
  for (auto &p : encoded_rates)
    output << p.first << "\t" << p.second << std::endl;

  output << "\nDistribution of the SI rates:" << std::endl;
  for (auto &p : si_rates)
    output << p.first << "\t" << p.second << std::endl;
}

boost::property_tree::ptree EncoderStatistics::toTree() const {
  boost::property_tree::ptree t;
  t.put("frames", total_frames);
  t.put("blocks", total_block);
  t.put("access_blocks", total_access_block);

  // histograms: syndrome length in bits -> count
  auto histogram = [](const std::map<float, unsigned long> &h) {
    boost::property_tree::ptree out;
    for (const auto &p : h) {
      boost::property_tree::ptree item;
      item.put("bits", p.first);
      item.put("count", p.second);
      out.push_back({"", item});
    }
    return out;
  };
  t.add_child("stored_rates", histogram(encoded_rates));
  t.add_child("prediction_rates", histogram(si_rates));
  return t;
}

unsigned long EncoderStatistics::getTotal_frames() const {
  return total_frames;
}

void EncoderStatistics::setTotal_frames(unsigned long newTotal_frames) {
  total_frames = newTotal_frames;
}

unsigned long EncoderStatistics::getTotal_block() const { return total_block; }

void EncoderStatistics::setTotal_block(unsigned long newTotal_block) {
  total_block = newTotal_block;
}

unsigned long EncoderStatistics::getTotal_access_block() const {
  return total_access_block;
}

void EncoderStatistics::setTotal_access_block(
    unsigned long newTotal_access_block) {
  total_access_block = newTotal_access_block;
}

unsigned long long EncoderStatistics::getTotal_bits_syndrome() const {
  return total_bits_syndrome;
}

void EncoderStatistics::setTotal_bits_syndrome(
    unsigned long long newTotal_bits_syndrome) {
  total_bits_syndrome = newTotal_bits_syndrome;
}

const std::map<float, unsigned long> &
EncoderStatistics::getAll_intra_rates() const {
  return encoded_rates;
}

const std::map<float, unsigned long> &
EncoderStatistics::getPredictions_rates() const {
  return si_rates;
}

unsigned long EncoderStatistics::getTemporal_not_used() const {
  return temporal_not_used;
}

void EncoderStatistics::setTemporal_not_used(
    unsigned long newTemporal_not_used) {
  temporal_not_used = newTemporal_not_used;
}

unsigned long EncoderStatistics::getTemporal_enabled() const {
  return temporal_enabled;
}

void EncoderStatistics::setTemporal_enabled(unsigned long newTemporal_enabled) {
  temporal_enabled = newTemporal_enabled;
}

} // namespace encoder
} // namespace icov
