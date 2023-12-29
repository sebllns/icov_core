#pragma once

#include "icov_console.h"
#include <iostream>
#include <memory>
#include <ostream>

#ifndef ICOV_LOG_LEVEL
#define ICOV_LOG_LEVEL 2
#endif

#ifdef ICOV_LOG_BOOST

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/format.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/shared_ptr.hpp>

#ifndef BOOST_LOG_DYN_LINK
#define BOOST_LOG_DYN_LINK 1
#endif

namespace icov {
namespace logging {

namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

inline void colored_formatter(boost::log::record_view const &rec,
                              boost::log::formatting_ostream &strm) {
  static auto date_time_formatter =
      expr::stream << expr::format_date_time<boost::posix_time::ptime>(
          "TimeStamp", "%Y-%m-%d %H:%M:%S.%f");

  auto severity = rec[boost::log::trivial::severity];

  switch (*severity) {
  case boost::log::trivial::trace:
    strm << CONSOLE_FORMAT(CONSOLE_DIM);
  case boost::log::trivial::debug:
    strm << CONSOLE_FORMAT(CONSOLE_FG_DEFAULT);
    break;
  case boost::log::trivial::info:
    strm << CONSOLE_FORMAT(CONSOLE_FG_GREEN);
    break;
  case boost::log::trivial::warning:
    strm << CONSOLE_FORMAT(CONSOLE_FG_YELLOW) << CONSOLE_FORMAT(CONSOLE_BOLD);
    break;
  case boost::log::trivial::error:
  case boost::log::trivial::fatal:
    strm << CONSOLE_FORMAT(CONSOLE_FG_RED) << CONSOLE_FORMAT(CONSOLE_BOLD);
    break;
  default:
    break;
  }

  //  strm << boost::log::extract<unsigned int>("LineID", rec) << ": [";
  date_time_formatter(rec, strm);
  // strm << "] [";

  strm << "[" << severity << "] \t" << rec[expr::message]
       << CONSOLE_FORMAT(CONSOLE_DEFAULT);
}

inline void set_filter(int lvl) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >= lvl);
}

inline void init_log(bool log_to_file) {

  boost::log::add_common_attributes();

  /* log formatter:
   * [TimeStamp] [ThreadId] [Severity Level] [Scope] Log message
   */
  auto fmtTimeStamp =
      boost::log::expressions::format_date_time<boost::posix_time::ptime>(
          "TimeStamp", "%Y-%m-%d %H:%M:%S.%f");

  auto fmtThreadId = boost::log::expressions::attr<
      boost::log::attributes::current_thread_id::value_type>("ThreadID");

  auto fmtSeverity =
      boost::log::expressions::attr<boost::log::trivial::severity_level>(
          "Severity");

  auto fmtScope = boost::log::expressions::format_named_scope(
      "Scope", boost::log::keywords::format = "%n(%f:%l)",
      boost::log::keywords::iteration = boost::log::expressions::reverse,
      boost::log::keywords::depth = 2);

  boost::log::formatter logFmt =
      boost::log::expressions::format("[%1%]\t(%2%)\t[%3%]\t[%4%]\t%5%") %
      fmtTimeStamp % fmtThreadId % fmtSeverity % fmtScope %
      boost::log::expressions::smessage;

  //  logging::add_file_log(
  //      keywords::file_name = "icov_%N.log",
  //      keywords::rotation_size = 200 * 1024 * 1024,
  //      // This makes the sink to write log records that look like this:
  //      // YYYY-MM-DD HH:MI:SS: <normal> A normal severity message
  //      // YYYY-MM-DD HH:MI:SS: <error> An error severity message
  //      keywords::format =
  //          (expr::stream << expr::format_date_time<boost::posix_time::ptime>(
  //                               "TimeStamp", "%Y-%m-%d %H:%M:%S")
  //                        << ": <" << logging::trivial::severity << "> "
  //                        << expr::smessage));

  /* console sink */
  auto consoleSink = boost::log::add_console_log(std::clog);
  consoleSink->set_formatter(&colored_formatter);

  if (log_to_file) {
    int rotation_size = 100 * 1024 * 1024;

    /* fs sink */
    auto fsSink = boost::log::add_file_log(
        boost::log::keywords::file_name = "icov_%Y-%m-%d_%H-%M-%S.%N.log",
        boost::log::keywords::rotation_size = rotation_size,
        boost::log::keywords::min_free_space = 3 * rotation_size,
        boost::log::keywords::open_mode = std::ios_base::app);
    fsSink->set_formatter(logFmt);
    fsSink->locked_backend()->auto_flush(true);
  }

  set_filter(ICOV_LOG_LEVEL);
}

} // namespace logging
} // namespace icov

#define ICOV_LOG(lvl) BOOST_LOG_TRIVIAL(lvl)
#define ICOV_TRACE ICOV_LOG(trace)
#define ICOV_DEBUG ICOV_LOG(debug)
#define ICOV_INFO ICOV_LOG(info)
#define ICOV_WARNING ICOV_LOG(warning)
#define ICOV_ERROR ICOV_LOG(error)
#define ICOV_FATAL ICOV_LOG(fatal)
#define ICOV_TODO ICOV_DEBUG << CONSOLE_TODO
#else

enum severity_level
{
    trace = 0,
    debug = 1,
    info = 2,
    warning = 3,
    error = 4,
    fatal = 5
};

namespace icov {
namespace logging {

inline std::ostream &get_log_str() { return std::cout; }
inline std::ostream &print(int lvl) {

  auto &str = get_log_str();

  switch (static_cast<severity_level>(lvl)) {
  case severity_level::trace:
    str << CONSOLE_FORMAT(CONSOLE_DEFAULT) << CONSOLE_FORMAT(CONSOLE_DIM)
        << "\n[trace]   ";
    break;
  case severity_level::debug:
      str << CONSOLE_FORMAT(CONSOLE_DEFAULT) << CONSOLE_FORMAT(CONSOLE_FG_DEFAULT) << "\n[debug]   ";
      break;
  case severity_level::info:
      str << CONSOLE_FORMAT(CONSOLE_DEFAULT) << CONSOLE_FORMAT(CONSOLE_FG_GREEN) << "\n[info]    ";
      break;
  case severity_level::warning:
      str << CONSOLE_FORMAT(CONSOLE_DEFAULT) << CONSOLE_FORMAT(CONSOLE_FG_YELLOW) << CONSOLE_FORMAT(CONSOLE_BOLD)
          << "\n[warning] ";
      break;
  case error:
    str << CONSOLE_FORMAT(CONSOLE_DEFAULT) << CONSOLE_FORMAT(CONSOLE_FG_RED)
        << CONSOLE_FORMAT(CONSOLE_BOLD) << "\n[error]   ";
    break;
  case severity_level::fatal:
    str << CONSOLE_FORMAT(CONSOLE_DEFAULT) << CONSOLE_FORMAT(CONSOLE_FG_RED)
        << CONSOLE_FORMAT(CONSOLE_BOLD) << "\n[fatal]   ";
    break;
  }

  return str;
}
inline void set_filter(int lvl) {}
inline void init_log(bool log_to_file) {}
} // namespace logging
} // namespace icov

#define ICOV_LOG(lvl)                                                          \
  if (lvl >= ICOV_LOG_LEVEL)                                                   \
  icov::logging::print(lvl)
#define ICOV_TRACE ICOV_LOG(0)
#define ICOV_DEBUG ICOV_LOG(1)
#define ICOV_INFO ICOV_LOG(2)
#define ICOV_WARNING ICOV_LOG(3)
#define ICOV_ERROR ICOV_LOG(4)
#define ICOV_FATAL ICOV_LOG(5)
#define ICOV_TODO ICOV_DEBUG << CONSOLE_TODO

#endif
