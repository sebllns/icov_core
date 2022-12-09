/****************************************************************************/
/** @addtogroup ICOV
 *  ICOV, open source Interactive Coder for Omnidirectional Videos.
 */

/** @addtogroup common
 *  @ingroup ICOV
 */

/** @file helpers.hpp
 *  @brief BRIEF_DESCRIPTION
 *  DESCRIPTION
 *  @ingroup common
 *  @author Sebastien Bellenous (SIROCCO) <sebastien.bellenous@inria.fr>
 *  @version 1.0.1
 *  @date 2022
 *  @copyright Copyright (C) 2022 by Inria. All rights reserved.
 * The copyright in this software is being made available under
 * the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 * <br/>
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * <br/>
 * <b> For using ICOV under terms that will not comply with the GNU
 * AGPL, please contact Inria about acquiring an ICOV Professional
 * Edition License. </b>
 * <br/>
 * See contact section in README file for more information.
 * <br/>
 * This software was developed at:
 * Inria Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 * <br/>
 * If you have questions regarding the use of this software, please contact
 * Inria using the contact section in the README file.
 * <br/>
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *****************************************************************************/
#pragma once

#include "typedef.h"
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES // for C++
#endif

#ifndef BOOST_ENABLE_ASSERT_DEBUG_HANDLER
#define BOOST_ENABLE_ASSERT_DEBUG_HANDLER
#endif

#ifndef BOOST_STACKTRACE_USE_ADDR2LINE
#define BOOST_STACKTRACE_USE_ADDR2LINE
#endif

#define stringify(name) #name

// two macros ensures any macro passed will
// be expanded before being stringified
#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iostream> // std::cerr
#include <numeric>
#include <sstream>
#include <stdexcept> // std::logic_error
#include <vector>

#include "common.h"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/exception/all.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/stacktrace.hpp>

#ifdef BOOST_LOG_DYN_LINK
#define BOOST_LOG_DYN_LINK 1
#endif

#define CONSOLE_FG_RED 91
#define CONSOLE_FG_GREEN 92
#define CONSOLE_FG_YELLOW 93
#define CONSOLE_FG_BLUE 94
#define CONSOLE_FG_DEFAULT 39
#define CONSOLE_BG_RED 41
#define CONSOLE_BG_GREEN 42
#define CONSOLE_BG_BLUE 44
#define CONSOLE_BG_DEFAULT 49
#define CONSOLE_BOLD 1
#define CONSOLE_DIM 2
#define CONSOLE_UNDERLINE 4
#define CONSOLE_HIGHLIGHT 7
#define CONSOLE_HIDDEN 8
#define CONSOLE_DEFAULT 0

#ifdef ICOV_CONSOLE_COLOR_ENABLED
#define CONSOLE_FORMAT(fmt) "\033[" STRINGIZE(fmt) "m"
#else
#define CONSOLE_FORMAT(fmt) ""
#endif

#define CONSOLE_TODO                                                           \
  CONSOLE_FORMAT(CONSOLE_FG_BLUE) CONSOLE_FORMAT(CONSOLE_HIGHLIGHT)

#define WRITE_CHARS(buf, data) buf.write((char *)&data, sizeof(data))

namespace boost {
inline void assertion_failed_msg(char const *expr, char const *msg,
                                 char const *function, char const * /*file*/,
                                 long /*line*/) {
  std::cerr << "Expression '" << expr << "' is false in function '" << function
            << "': " << (msg ? msg : "<...>") << ".\n"
            << "Backtrace:\n"
            << boost::stacktrace::stacktrace() << '\n';
  std::abort();
}

inline void assertion_failed(char const *expr, char const *function,
                             char const *file, long line) {
  ::boost::assertion_failed_msg(expr, 0 /*nullptr*/, function, file, line);
}
} // namespace boost

namespace icov::helpers {

template <class ForwardIt, class T>
void iota(ForwardIt first, ForwardIt last, T value, T step) {
  while (first != last) {
    *first++ = value;
    value += step;
  }
}

template <class T> std::vector<T> iota(T first, T last, T step) {
  std::vector<T> v;

  if (first < last)
    assert(step > 0);
  if (first > last)
    assert(step < 0);

  T value = first;
  while (value != last) {
    v.push_back(value);
    value += step;
  }
  v.push_back(last);
  return v;
}

template <class T> std::vector<T> iota(T first, T step, int count) {
  return iota(first, first + step * (count - 1), step);
}

template <typename T>
bool lazy_compare_vector(const std::vector<T> &v1, const std::vector<T> &v2) {
  if (v1.size() != v2.size()) {
    return false;
  }

  bool equal = true;
  int i = -1;

  while (equal && ++i < (int)v1.size()) {
    equal = (v1[i] == v2[i]);
  }

  return equal;
}

template <typename T> FastMatrix<T> transpose(const FastMatrix<T> &m) {
  FastMatrix<T> t(m.cols(), m.rows());
  for (int i = 0; i < t.rows(); i++) {
    for (int j = 0; j < t.cols(); j++) {
      t.at(i, j) = m.at(j, i);
    }
  }
  return t;
}

namespace exception {
typedef boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace>
    traced;

template <class E> void throw_with_trace(const E &e) {
  BOOST_THROW_EXCEPTION(boost::enable_error_info(e)
                        << traced(boost::stacktrace::stacktrace()));
}

#if !defined(NDEBUG) || defined(ICOV_CORE_DEBUG_TRACE_ENABLED)

#define ICOV_THROW_EXCEPTION(e) icov::helpers::exception::throw_with_trace(e)
#else
#define ICOV_THROW_EXCEPTION(e) BOOST_THROW_EXCEPTION(e)
#endif

#define TO_STR(MAC)                                                            \
  ([=] {                                                                       \
    std::stringstream ss;                                                      \
    ss << MAC;                                                                 \
    return std::string(ss.str());                                              \
  })()

#define CURRENT_LOCATION std::source_location::current()

#define ICOV_DEBUG_HEADER(msg)                                                 \
  TO_STR(CONSOLE_FORMAT(CONSOLE_BOLD)                                          \
         << "In file " << __FILE__ << ":" << __LINE__ << " "                   \
         << CONSOLE_FORMAT(CONSOLE_FG_RED) << msg                              \
         << CONSOLE_FORMAT(CONSOLE_DEFAULT))

//#define ICOV_DEBUG_HEADER(msg)                                                 \
//  TO_STR("In file " << CONSOLE_FORMAT(CONSOLE_BOLD) << __FILE__ << ":"         \
//                    << __LINE__ << " " << CONSOLE_FORMAT(CONSOLE_DEFAULT)      \
//                    << CONSOLE_FORMAT(CONSOLE_FG_RED) << msg                   \
//                    << CONSOLE_FORMAT(CONSOLE_DEFAULT))

#define ICOV_THROW_LOGICAL_EXCEPTION(msg)                                      \
  ICOV_THROW_EXCEPTION(std::logic_error(ICOV_DEBUG_HEADER(msg)))
#define ICOV_THROW_RUNTIME_EXCEPTION(msg)                                      \
  ICOV_THROW_EXCEPTION(std::runtime_error(ICOV_DEBUG_HEADER(msg)))
#define ICOV_THROW_INVALID_ARGUMENT(msg)                                       \
  ICOV_THROW_EXCEPTION(std::invalid_argument(ICOV_DEBUG_HEADER(msg)))
#define ICOV_THROW_OUT_OF_RANGE(msg)                                           \
  ICOV_THROW_EXCEPTION(std::range_error(ICOV_DEBUG_HEADER(msg)))
#define ICOV_THROW_NULLPTR(msg)                                                \
  ICOV_THROW_EXCEPTION(std::invalid_argument(ICOV_DEBUG_HEADER(msg)))

} // namespace exception

namespace assert {

#ifndef NDEBUG
#define ICOV_ASSERT_MSG(cond, msg)                                             \
  {                                                                            \
    if (!(cond)) {                                                             \
      std::cerr << "*******************************************************"   \
                << std::endl                                                   \
                << msg << std::endl;                                           \
    }                                                                          \
    BOOST_ASSERT(cond);                                                        \
  }

#define ICOV_WARNING_MSG(cond, msg)                                            \
  {                                                                            \
    if (!(cond)) {                                                             \
      std::cerr << "*******************************************************"   \
                << std::endl                                                   \
                << "WARNING: " << msg << std::endl;                            \
    }                                                                          \
  }

#else
#define ICOV_ASSERT_MSG(...)
#define ICOV_WARNING_MSG(...)
#endif

template <typename T> void range(T val, T min, T max) {
  if (min > max)
    BOOST_THROW_EXCEPTION(std::invalid_argument("Required min <= max"));

  if (val < min || val > max)
    BOOST_THROW_EXCEPTION(std::out_of_range("VALUE=" + std::to_string(val) +
                                            " while the acceptable range "
                                            "is [" +
                                            std::to_string(min) + ", " +
                                            std::to_string(max) + "]"));
}

void not_null(const void *p,
              const std::string &msg = "Pointer was not allocated");
void check(bool expr, const std::string &msg);
template <typename T>
void equal(T a, T b, const std::string &msg = "elements must be equal") {
  check(a == b, msg);
}

} // namespace assert

namespace maths {
int pow2(int k);
uint32_t binary_length(int val);
float normalize(float val, float min, float max);
float wrap(float val, float min, float max);
float repeat(float val, float min, float max);
int quantize(int value, int step, int min, int max);
int quantize(float value, const std::vector<float> &v);

} // namespace maths

namespace bitstream {

template <typename T> icov::byte boolToByte(const std::array<T, 8> in) {
  icov::byte result = 0;
  int i = 0;

  for (auto &v : in) {
    result = (result << 1);
    if (static_cast<bool>(in[i++]))
      result++;
  }
  return result;
}

bool getBit(int bit_pos, icov::byte b);
} // namespace bitstream

namespace rand {
void init();
int range(int min_included, int max_included);
float normalized();
} // namespace rand

namespace io {

template <typename T>
std::ostream &write_vector(std::ostream &out, const std::vector<T> &data,
                           char sep) {
  out << data.size();
  for (int i = 0; i < data.size(); i++) {
    out << sep << data[i];
  }
  return out;
}

template <typename T>
void write_vector(const std::string &filename, const std::vector<T> &data,
                  char sep) {
  std::ofstream file(filename);
  write_vector(file, data, sep);
  file.close();
}

template <typename T>
void read_vector(const std::string &filename, std::vector<T> &data) {
  std::ifstream file(filename);

  if (!file.is_open()) {
    throw std::invalid_argument("cannot open file " + filename);
  }

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

inline void cls(void) {
  try {
    system("cls||clear");
  } catch (...) {
  }
  return;
}

} // namespace io

namespace path {
bool file_exists(const std::string &path);
bool directory_exists(const std::string &path);
void assert_file_exists(const std::string &path);
void assert_directory_exists(const std::string &path);
bool try_create_directories(const std::string &path);
std::string get_data_folder(const std::string &root, int code_size,
                            bool auto_create = false);
std::string combine(const std::string &path1, const std::string &path2);
} // namespace path

} // namespace icov::helpers
