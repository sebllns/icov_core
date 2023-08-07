#pragma once

#include "icov_assert.h"
#include "icov_log.h"
#include <boost/filesystem.hpp>
#include <string>

namespace fs = boost::filesystem;

namespace icov {
namespace path {

inline bool try_create_directories(const std::string &path) {
  bool created = false;
  try {
    created = fs::create_directories(path);
  } catch (...) {
    created = false;
    ICOV_ERROR << "Error: could not create directory " << path << std::endl;
    ICOV_ERROR << boost::current_exception_diagnostic_information()
               << std::endl;
  }
  return created;
}

inline std::string combine(const std::string &path1, const std::string &path2) {
  return fs::absolute(fs::path(path1).append(path2).make_preferred()).string();
}

inline bool file_exists(const std::string &path) {
  return fs::is_regular_file(fs::absolute(path).make_preferred());
}

inline bool directory_exists(const std::string &path) {
  return fs::is_directory(fs::absolute(path).make_preferred());
}

inline void assert_file_exists(const std::string &path) {
  icov::assert::check(file_exists(path), "Path is a valid file: '" + path + "'");
}

inline void assert_directory_exists(const std::string &path) {
  icov::assert::check(directory_exists(path),
                      "Path is a valid directory: " + path);
}

inline std::string get_data_folder(const std::string &root, int code_size,
                                   bool auto_create) {
  fs::path path(root);
  path /= std::to_string(code_size);

  path = fs::absolute(path).make_preferred();
  if (auto_create) {
    try_create_directories(path.string());
  }
  return path.string();
}
} // namespace path
} // namespace icov
