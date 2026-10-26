#include "matrix_set.h"

#include "icov_exception.h"
#include "video_layer/bit_io.h"

#include <iomanip>
#include <sstream>

namespace icov {
namespace bitstream {

namespace {

void write_string(BitWriter &w, const std::string &s) {
  w.ue(s.size());
  for (char c : s)
    w.u(8, static_cast<uint8_t>(c));
}

std::string read_string(BitReader &r) {
  std::string s(r.ue(), '\0');
  for (auto &c : s)
    c = static_cast<char>(r.u(8));
  return s;
}

std::string hex(uint32_t v) {
  std::ostringstream s;
  s << std::hex << std::setw(8) << std::setfill('0') << v;
  return s.str();
}

} // namespace

std::vector<uint8_t> write_matrix_set(const std::string &name,
                                      const data::FileSet &files) {
  BitWriter w;
  write_string(w, name);
  w.u(32, data::set_crc32(files));
  w.ue(files.size());
  for (const auto &f : files) {
    std::vector<uint64_t> values;
    std::istringstream in(f.second);
    long long v;
    while (in >> v) {
      if (v < 0)
        ICOV_THROW_INVALID_ARGUMENT("Negative value in " + f.first);
      values.push_back(uint64_t(v));
    }
    unsigned width = 0;
    for (uint64_t x : values)
      while ((x >> width) != 0)
        ++width;

    write_string(w, f.first);
    w.ue(values.size());
    w.u(5, width);
    for (uint64_t x : values)
      w.u(width, x);
  }
  w.byte_alignment();
  return w.data();
}

data::FileSet read_matrix_set(const std::vector<uint8_t> &payload,
                              std::string &name, uint32_t &crc) {
  BitReader r(payload);
  name = read_string(r);
  crc = r.u(32);
  data::FileSet files;
  const size_t count = r.ue();
  for (size_t i = 0; i < count; ++i) {
    const std::string path = read_string(r);
    const size_t n = r.ue();
    const unsigned width = r.u(5);
    std::string text;
    for (size_t k = 0; k < n; ++k)
      text += std::to_string(r.u(width)) + (k + 1 < n ? " " : "\n");
    files[path] = text;
  }
  if (data::set_crc32(files) != crc)
    ICOV_THROW_RUNTIME_EXCEPTION("Corrupted LDPC matrix set " + name);
  return files;
}

std::string resolve_matrix_set(const ContentParameterSet &cps,
                               const std::string &stored_root,
                               const std::string &user_root) {
  const std::string builtin = data::DEFAULT_MATRIX_SET;

  // 2.0.0 files carry no CRC: the built-in set, or -d when given
  if (cps.ldpc_matrix_crc == 0)
    return user_root;

  std::vector<std::string> candidates;
  if (!stored_root.empty())
    candidates.push_back(stored_root);
  candidates.push_back(builtin);
  if (user_root != builtin)
    candidates.push_back(user_root);

  for (const auto &root : candidates) {
    try {
      if (data::set_crc32(data::load_set(root)) == cps.ldpc_matrix_crc)
        return root;
    } catch (const std::exception &) {
      // missing directory: try the next candidate
    }
  }
  ICOV_THROW_INVALID_ARGUMENT(
      "The file was encoded with the LDPC matrix set " + cps.ldpc_matrix_set +
      " (crc " + hex(cps.ldpc_matrix_crc) +
      "), which is neither stored in the file nor built in: pass its "
      "directory with -d");
  return builtin;
}

} // namespace bitstream
} // namespace icov
