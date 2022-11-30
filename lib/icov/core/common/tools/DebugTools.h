#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/log/trivial.hpp>
#define ICOV_LOG(lvl) BOOST_LOG_TRIVIAL(lvl)

#ifndef ICOV_LOG_LEVEL
#define ICOV_LOG_LEVEL 2
#endif

namespace icov {

namespace debug {

static const bool debug_trace_enabled =
#ifdef ICOV_CORE_DEBUG_TRACE_ENABLED
    true;
#else
    false;
#endif
static const bool is_debug_build =
#ifdef NDEBUG
    false;
#else
    true;
#endif

static const bool is_release_build = !is_debug_build;

// see boost doc to config log file
// https://www.boost.org/doc/libs/1_65_1/libs/log/doc/html/log/tutorial/sinks.html
void initLog();

void write_str(const std::string &filename, const std::string &str);

template <typename T>
static void write_vector(const std::string &filename,
                         const std::vector<T> &data, char sep) {
  std::ofstream file(filename);
  file << data.size();
  for (int i = 0; i < data.size(); i++) {
    file << sep << data[i];
  }
  file.close();
}

template <typename T>
static void read_vector(const std::string &filename, std::vector<T> &data) {
  std::ifstream file(filename);
  int n = 0;
  file >> n;
  data = std::vector<T>(n);

  T val;
  for (int i = 0; i < data.size(); i++) {
    file >> val;
    data[i] = val;
  }
  file.close();
}

template <typename T>
static bool compare_vector(const std::vector<T> &v1, const std::vector<T> &v2) {
  if (v1.size() != v2.size()) {
    std::cout << "different size\n";
    return false;
  }

  bool equal = true;

  for (int i = 0; i < v1.size(); i++) {
    if (v1[i] != v2[i]) {
      equal = false;
      std::cout << i << " ";
    }
  }

  std::cout << std::endl
            << "vectors are " << (equal ? "equal\n" : "not equal\n");

  return equal;
}

template <typename T>
static void print_array(const T *arr, int n, const std::string &name) {

  std::cout << "\n\n*********************************************************"
               "***********************************\n"
            << array_tostring(arr, n, name);

  // getchar();
}

template <typename T>
static std::string array_tostring(const T *arr, int n,
                                  const std::string &name) {
  std::stringstream sstr;

  sstr << name << "[";
  for (int i = 0; i < n - 1; i++) {
    sstr << arr[i] << ", ";
  }

  sstr << arr[n - 1] << "]\n";

  return sstr.str();
}

//  template <typename T>
//  static void print_table(std::ostream &out,
//                          const std::vector<std::string> &header_col,
//                          const std::vector<std::string> &header_row,
//                          const std::vector<int> &col_width,
//                          const std::vector<std::vector<T>> &data,
//                          const std::string data_format) {

//    int ncol = header_row.size();
//    int nrow = header_col.size() - 1;

//    for (int j = 0; j <= ncol; j++) {

//      std::cout << boost::format("%=" + std::to_string(col_width[j])) %
//                       header_row[j];
//    }

//    for (int i = 0; i <= nrow; i++) {
//      for (int j = 0; j <= ncol; j++) {
//        if (i == 0 && j > 0) {
//          // header_row j - 1
//          std::cout << boost::format("%=" + std::to_string(col_width[j])) %
//                           header_row[j - 1];
//        } else if (j == 0 && i > 0) {
//          // header_col i - 1
//          std::cout << boost::format("%_" + std::to_string(col_width[j])) %
//                           header_row[j - 1];
//        } else if (i > 0 && j > 0) {
//          // data (i - 1) (j - 1)
//        } else {
//        }
//      }
//    }
//  }

} // namespace debug
} // namespace icov

namespace DebugTools = icov::debug;
