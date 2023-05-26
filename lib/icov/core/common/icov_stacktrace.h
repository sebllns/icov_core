#pragma once

#ifndef BOOST_STACKTRACE_USE_ADDR2LINE
#define BOOST_STACKTRACE_USE_ADDR2LINE
#endif

#include <boost/exception/diagnostic_information.hpp>
#include <boost/stacktrace.hpp>

#define ICOV_FILE_INDICATOR std::string(__FILE__)
#define ICOV_LINE_INDICATOR std::to_string(__LINE__)
#define ICOV_INDICATOR ICOV_FILE_INDICATOR + ":" + ICOV_LINE_INDICATOR + " "
#define ICOV_INDICATOR_MSG(msg) ICOV_INDICATOR + std::string(msg)

namespace icov {
namespace stacktrace {}
} // namespace icov
