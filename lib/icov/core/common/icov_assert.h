#pragma once

#include "icov_exception.h"
#include "icov_log.h"
#include <cstdlib>
#include <iostream>

// If the macro BOOST_ENABLE_ASSERT_DEBUG_HANDLER is defined when
// <boost/assert.hpp> is included, BOOST_ASSERT_MSG(expr) expands to ((void)0)
// when NDEBUG is defined. Otherwise the behavior is as if
// BOOST_ENABLE_ASSERT_HANDLER has been defined.
#ifndef BOOST_ENABLE_ASSERT_DEBUG_HANDLER
#define BOOST_ENABLE_ASSERT_DEBUG_HANDLER
#endif

namespace boost {
inline void assertion_failed_msg(char const *expr, char const *msg,
                                 char const *function, char const * /*file*/,
                                 long /*line*/) {
  ICOV_FATAL << "Expression '" << expr << "' is false in function '" << function
             << "': " << (msg ? msg : "<...>") << ".\n";
#ifdef ICOV_CORE_STACKTRACE_ENABLED
  ICOV_FATAL << "Backtrace:\n" << boost::stacktrace::stacktrace() << '\n';
#endif
  std::abort();
}

inline void assertion_failed(char const *expr, char const *function,
                             char const *file, long line) {
  ::boost::assertion_failed_msg(expr, 0 /*nullptr*/, function, file, line);
}
} // namespace boost

namespace icov {
namespace assert {

#ifndef NDEBUG
#define ICOV_ASSERT_MSG(cond, msg)                                             \
  {                                                                            \
    if (!(cond)) {                                                             \
      ICOV_FATAL << msg;                                                       \
    }                                                                          \
    BOOST_ASSERT(cond);                                                        \
  }

#define ICOV_WARNING_MSG(cond, msg)                                            \
  {                                                                            \
    if (!(cond)) {                                                             \
      ICOV_WARNING << msg;                                                     \
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

inline void check(bool expr,
                  const std::string &msg = "Check expression is true") {
  if (!expr) {
#ifdef ICOV_CORE_STACKTRACE_ENABLED
    ICOV_FATAL << boost::stacktrace::stacktrace();
#endif
    ICOV_THROW_LOGICAL_EXCEPTION("Assertion is invalid: " + msg);
  }
}

inline void not_null(const void *p,
                     const std::string &msg = "Pointer cannot be null") {
  if (!p) {
#ifdef ICOV_CORE_STACKTRACE_ENABLED
    ICOV_FATAL << boost::stacktrace::stacktrace();
#endif
    ICOV_THROW_NULLPTR(msg);
  }
}

template <typename T>
void equal(T a, T b, const std::string &msg = "Elements must be equal") {
  check(a == b, msg);
}

} // namespace assert
} // namespace icov
