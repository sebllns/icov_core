#include "DebugTools.h"

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
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/shared_ptr.hpp>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

void icov::debug::initLog() {

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
      boost::log::expressions::format("[%1%] (%2%) [%3%] [%4%] %5%") %
      fmtTimeStamp % fmtThreadId % fmtSeverity % fmtScope %
      boost::log::expressions::smessage;

  logging::add_file_log(
      keywords::file_name = "icov_%N.log",
      keywords::rotation_size = 200 * 1024 * 1024,
      // This makes the sink to write log records that look like this:
      // YYYY-MM-DD HH:MI:SS: <normal> A normal severity message
      // YYYY-MM-DD HH:MI:SS: <error> An error severity message
      keywords::format =
          (expr::stream << expr::format_date_time<boost::posix_time::ptime>(
                               "TimeStamp", "%Y-%m-%d %H:%M:%S")
                        << ": <" << logging::trivial::severity << "> "
                        << expr::smessage));

  /* console sink */
  auto consoleSink = boost::log::add_console_log(std::clog);
  consoleSink->set_formatter(logFmt);

  int rotation_size = 100 * 1024 * 1024;

  /* fs sink */
  auto fsSink = boost::log::add_file_log(
      boost::log::keywords::file_name = "test_%Y-%m-%d_%H-%M-%S.%N.log",
      boost::log::keywords::rotation_size = rotation_size,
      boost::log::keywords::min_free_space = 3 * rotation_size,
      boost::log::keywords::open_mode = std::ios_base::app);
  fsSink->set_formatter(logFmt);
  fsSink->locked_backend()->auto_flush(true);

  logging::core::get()->set_filter(logging::trivial::severity >=
                                   ICOV_LOG_LEVEL);

  ICOV_LOG_MSG(info) << "Log intialized";
}

void icov::debug::write_str(const std::string &filename,
                            const std::string &str) {
  std::ofstream file(filename);
  file << str;
  file.close();
}
