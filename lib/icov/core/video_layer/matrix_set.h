#pragma once

#include "icov_data.h"
#include "video_layer/ContentParameterSet.h"

#include <cstdint>
#include <string>
#include <vector>

namespace icov {
namespace bitstream {

/// Payload of the optional 'ildp' box: the LDPC matrix set stored in the file
///   set_name                     ue(v) length + u(8) chars
///   set_crc                      u(32)  icov::data::set_crc32
///   file_count                   ue(v)
///   per file:
///     path                       ue(v) length + u(8) chars
///     value_count                ue(v)
///     bit_width                  u(5)
///     values                     u(bit_width) each
/// All set files are whitespace-separated non-negative integers.
std::vector<uint8_t> write_matrix_set(const std::string &name,
                                      const data::FileSet &files);

/// Parses the payload; returns the files as text, and the name and CRC
data::FileSet read_matrix_set(const std::vector<uint8_t> &payload,
                              std::string &name, uint32_t &crc);

/// Chooses the matrix set matching the parameter set, in this order: the set
/// stored in the file (stored_root, may be empty), the built-in set, then the
/// -d directory (user_root, the built-in set when -d was not given).
/// Throws with the set name and CRC when none matches.
std::string resolve_matrix_set(const ContentParameterSet &cps,
                               const std::string &stored_root,
                               const std::string &user_root);

} // namespace bitstream
} // namespace icov
