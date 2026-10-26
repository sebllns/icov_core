#pragma once

#include "icov_console.h"
#include "icov_stacktrace.h"

#include <boost/exception/all.hpp>

//#include <boost/exception/detail/error_info_impl.hpp>
//#include <boost/exception/exception.hpp>

namespace icov {
namespace exception {

/**
 * @brief Type definition for traced stacktrace information.
 */
typedef boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace>
    traced;

/**
 * @brief Throws an exception with a stacktrace.
 *
 * This function throws the given exception with an attached stacktrace.
 *
 * @param e The exception to be thrown.
 */
template <class E> void throw_with_trace(const E &e) {
  BOOST_THROW_EXCEPTION(boost::enable_error_info(e)
                        << traced(boost::stacktrace::stacktrace()));
}

#if !defined(NDEBUG) || defined(ICOV_CORE_STACKTRACE_ENABLED)
/**
 * @def ICOV_THROW_EXCEPTION(e)
 * @brief Macro to throw an exception with a stacktrace.
 *
 * This macro throws the given exception with an attached stacktrace if
 * debugging or stacktrace is enabled. Otherwise, it simply throws the
 * exception.
 *
 * @param e The exception to be thrown.
 */
#define ICOV_THROW_EXCEPTION(e) icov::exception::throw_with_trace(e)
#else
#define ICOV_THROW_EXCEPTION(e) BOOST_THROW_EXCEPTION(e)
#endif

/**
 * @def TO_STR(MAC)
 * @brief Macro to convert a macro to a string.
 *
 * This macro converts the given macro to a string.
 *
 * @param MAC The macro to convert.
 * @return The string representation of the macro.
 */
#define TO_STR(MAC)                                                            \
  ([=] {                                                                       \
    std::stringstream ss;                                                      \
    ss << MAC;                                                                 \
    return std::string(ss.str());                                              \
  })()

#define CURRENT_LOCATION std::source_location::current()

/**
 * @def ICOV_DEBUG_HEADER(msg)
 * @brief Macro to create a debug header string.
 *
 * This macro creates a debug header string with the file name, line number, and
 * additional message.
 *
 * @param msg The additional message to include in the debug header.
 * @return The formatted debug header string.
 */
#define ICOV_DEBUG_HEADER(msg)                                                                                         \
  TO_STR(CONSOLE_FORMAT(CONSOLE_BOLD) << "In file " << __FILE__ << ":" << __LINE__ << " "                              \
                                      << CONSOLE_FORMAT(CONSOLE_FG_RED) << msg << CONSOLE_FORMAT(CONSOLE_DEFAULT))

#define ICOV_THROW_STD_EXCEPTION(msg) ICOV_THROW_EXCEPTION(std::runtime_error(ICOV_DEBUG_HEADER(msg)))

/**
 * @def ICOV_THROW_LOGICAL_EXCEPTION(msg)
 * @brief Macro to throw a logical exception.
 *
 * This macro throws a logical exception with a debug header message.
 *
 * @param msg The additional message to include in the exception.
 */
#define ICOV_THROW_LOGICAL_EXCEPTION(msg)                                      \
  ICOV_THROW_EXCEPTION(std::logic_error(ICOV_DEBUG_HEADER(msg)))

/**
 * @def ICOV_THROW_RUNTIME_EXCEPTION(msg)
 * @brief Macro to throw a runtime exception.
 *
 * This macro throws a runtime exception with a debug header message.
 *
 * @param msg The additional message to include in the exception.
 */
#define ICOV_THROW_RUNTIME_EXCEPTION(msg)                                      \
  ICOV_THROW_EXCEPTION(std::runtime_error(ICOV_DEBUG_HEADER(msg)))

/**
 * @def ICOV_THROW_INVALID_ARGUMENT(msg)
 * @brief Macro to throw an invalid argument exception.
 *
 * This macro throws an invalid argument exception with a debug header message.
 *
 * @param msg The additional message to include in the exception.
 */
#define ICOV_THROW_INVALID_ARGUMENT(msg)                                       \
  ICOV_THROW_EXCEPTION(std::invalid_argument(ICOV_DEBUG_HEADER(msg)))

/**
 * @def ICOV_THROW_OUT_OF_RANGE(msg)
 * @brief Macro to throw an out-of-range exception.
 *
 * This macro throws an out-of-range exception with a debug header message.
 *
 * @param msg The additional message to include in the exception.
 */
#define ICOV_THROW_OUT_OF_RANGE(msg)                                           \
  ICOV_THROW_EXCEPTION(std::range_error(ICOV_DEBUG_HEADER(msg)))

/**
 * @def ICOV_THROW_NULLPTR(msg)
 * @brief Macro to throw a null pointer exception.
 *
 * This macro throws a null pointer exception with a debug header message.
 *
 * @param msg The additional message to include in the exception.
 */
#define ICOV_THROW_NULLPTR(msg)                                                \
  ICOV_THROW_EXCEPTION(std::invalid_argument(ICOV_DEBUG_HEADER(msg)))

#define CATCH_ALL(msg)                                                                                                 \
  catch (const std::exception &e)                                                                                      \
  {                                                                                                                    \
      ICOV_ERROR << msg << ": " << e.what();                                                                           \
  }                                                                                                                    \
  catch (...)                                                                                                          \
  {                                                                                                                    \
      ICOV_ERROR << msg << ": " << boost::current_exception_diagnostic_information();                                  \
  }

} // namespace exception
} // namespace icov
