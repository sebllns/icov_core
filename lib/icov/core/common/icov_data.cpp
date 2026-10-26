#include "icov_data.h"

#include <boost/crc.hpp>
#include <boost/filesystem.hpp>

#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace fs = boost::filesystem;

namespace icov {
namespace data {

/// Generated at configure time from data/ (see cmake/EmbedData.cmake)
const std::map<std::string, std::string> &builtin_files();

namespace {

const char BUILTIN[] = "builtin:";
const char STREAM[] = "stream:";

bool starts_with(const std::string &s, const char *prefix) {
  return s.rfind(prefix, 0) == 0;
}

std::mutex &registry_mutex() {
  static std::mutex m;
  return m;
}

std::map<std::string, FileSet> &registry() {
  static std::map<std::string, FileSet> r;
  return r;
}

/// Splits "builtin:imt_164_v2/1024/H.pcm" into the root and the relative path
void split(const std::string &path, std::string &root, std::string &rel) {
  const size_t colon = path.find(':');
  const size_t slash = path.find('/', colon);
  root = path.substr(0, slash);
  rel = slash == std::string::npos ? "" : path.substr(slash + 1);
  // normalize "a//b" and "./" left by path concatenations
  std::string out;
  std::istringstream parts(rel);
  std::string part;
  while (std::getline(parts, part, '/')) {
    if (part.empty() || part == ".")
      continue;
    out += (out.empty() ? "" : "/") + part;
  }
  rel = out;
}

const std::string *find_virtual(const std::string &path) {
  std::string root, rel;
  split(path, root, rel);
  if (starts_with(root, BUILTIN)) {
    const auto &files = builtin_files();
    auto it = files.find(root.substr(sizeof(BUILTIN) - 1) + "/" + rel);
    return it == files.end() ? nullptr : &it->second;
  }
  std::lock_guard<std::mutex> lock(registry_mutex());
  auto set = registry().find(root);
  if (set == registry().end())
    return nullptr;
  auto it = set->second.find(rel);
  return it == set->second.end() ? nullptr : &it->second;
}

} // namespace

bool is_virtual(const std::string &path) {
  return starts_with(path, BUILTIN) || starts_with(path, STREAM);
}

std::unique_ptr<std::istream> open(const std::string &path) {
  if (is_virtual(path)) {
    const std::string *content = find_virtual(path);
    if (content)
      return std::make_unique<std::istringstream>(*content);
  } else {
    auto file = std::make_unique<std::ifstream>(path);
    if (file->is_open())
      return file;
  }
  throw std::runtime_error("'filename' couldn't be opened ('filename' = " +
                           path + ").");
}

bool exists(const std::string &path) {
  return is_virtual(path) ? find_virtual(path) != nullptr
                          : fs::is_regular_file(path);
}

void register_set(const std::string &root, FileSet files) {
  std::lock_guard<std::mutex> lock(registry_mutex());
  registry()[root] = std::move(files);
}

FileSet load_set(const std::string &root) {
  FileSet set;
  if (starts_with(root, BUILTIN)) {
    const std::string prefix = root.substr(sizeof(BUILTIN) - 1) + "/";
    for (const auto &f : builtin_files())
      if (f.first.rfind(prefix, 0) == 0)
        set[f.first.substr(prefix.size())] = f.second;
  } else if (starts_with(root, STREAM)) {
    std::lock_guard<std::mutex> lock(registry_mutex());
    auto it = registry().find(root);
    if (it != registry().end())
      set = it->second;
  } else if (fs::is_directory(root)) {
    for (const auto &e : fs::recursive_directory_iterator(root)) {
      if (!fs::is_regular_file(e.path()))
        continue;
      std::ifstream in(e.path().string(), std::ios::binary);
      std::ostringstream text;
      text << in.rdbuf();
      set[fs::relative(e.path(), root).generic_string()] = text.str();
    }
  }
  if (set.empty())
    throw std::runtime_error("No data files found in " + root);
  return set;
}

std::string set_name(const std::string &root) {
  if (is_virtual(root))
    return root.substr(root.find(':') + 1);
  fs::path p(root);
  if (p.filename() == "." || p.filename().empty())
    p = p.parent_path();
  return p.filename().string();
}

uint32_t set_crc32(const FileSet &files) {
  boost::crc_32_type crc;
  for (const auto &f : files) {
    crc.process_bytes(f.first.data(), f.first.size() + 1);
    std::istringstream in(f.second);
    long long v;
    while (in >> v) {
      uint8_t le[8]; // fixed byte order, so the CRC is portable
      for (int i = 0; i < 8; ++i)
        le[i] = uint8_t(uint64_t(v) >> (8 * i));
      crc.process_bytes(le, sizeof(le));
    }
  }
  return crc.checksum();
}

} // namespace data
} // namespace icov
