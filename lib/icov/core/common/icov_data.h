#pragma once

#include <cstdint>
#include <istream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace icov {
namespace data {

/// Data files (LDPC matrices, q-ary table) are read through this module from
/// three kinds of roots:
///   a directory                  -d <dir>
///   builtin:<name>               compiled into the library (imt_164_v2, q-ary)
///   stream:<crc>                 matrices stored in an ICOV file (ildp box)
/// A path is a root followed by a relative path, e.g. builtin:imt_164_v2/1024/H.pcm

/// Default LDPC matrix set, compiled into the library
constexpr char DEFAULT_MATRIX_SET[] = "builtin:imt_164_v2";

/// Files of a set: relative path -> text content
using FileSet = std::map<std::string, std::string>;

bool is_virtual(const std::string &path);

/// Opens a data file; throws std::runtime_error when it does not exist
std::unique_ptr<std::istream> open(const std::string &path);
bool exists(const std::string &path);

/// Registers an in-memory set under a virtual root (e.g. stream:<crc>)
void register_set(const std::string &root, FileSet files);

/// All files of a root, read recursively for a directory
FileSet load_set(const std::string &root);

/// Set name: directory base name or the part after "builtin:"
std::string set_name(const std::string &root);

/// CRC-32 of a set, over the relative paths and the integers of each file,
/// so that it does not depend on text formatting
uint32_t set_crc32(const FileSet &files);

} // namespace data
} // namespace icov
