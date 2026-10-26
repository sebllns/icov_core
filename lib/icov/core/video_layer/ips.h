#pragma once

#include "video_layer/ContentParameterSet.h"
#include "video_layer/bit_io.h"

#include <cstdint>
#include <vector>

namespace icov {
namespace bitstream {

/// Version of the ICOV parameter set syntax
constexpr uint8_t IPS_FORMAT_VERSION = 2;

/// Writes the ICOV parameter set (sequence parameters) as an RBSP
void write_ips(BitWriter &w, const ContentParameterSet &cps);
std::vector<uint8_t> write_ips(const ContentParameterSet &cps);

/// Parses an ICOV parameter set written by write_ips
ContentParameterSet read_ips(BitReader &r);
ContentParameterSet read_ips(const std::vector<uint8_t> &data);

} // namespace bitstream
} // namespace icov
