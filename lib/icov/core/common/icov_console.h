#pragma once

#include <cstdlib>

namespace icov {
namespace console {

/**
 * @def stringify(name)
 * @brief Macro to convert a macro name to a string literal.
 *
 * This macro converts the given macro name to a string literal.
 *
 * @param name The macro to convert.
 * @return The string literal representation of the macro name.
 */
#define stringify(name) #name

// two macros ensures any macro passed will
// be expanded before being stringified
#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

/**
 * @brief Constants for console text color.
 */
#define CONSOLE_FG_RED 91 /**< Red text color. */
#define CONSOLE_FG_GREEN 92 /**< Green text color. */
#define CONSOLE_FG_YELLOW 93 /**< Yellow text color. */
#define CONSOLE_FG_BLUE 94 /**< Blue text color. */
#define CONSOLE_FG_DEFAULT 39 /**< Default text color. */
#define CONSOLE_BG_RED 41 /**< Red background color. */
#define CONSOLE_BG_GREEN 42 /**< Green background color. */
#define CONSOLE_BG_BLUE 44 /**< Blue background color. */
#define CONSOLE_BG_DEFAULT 49 /**< Default background color. */
#define CONSOLE_BOLD 1 /**< Bold text style. */
#define CONSOLE_DIM 2 /**< Dim text style. */
#define CONSOLE_UNDERLINE 4 /**< Underline text style. */
#define CONSOLE_HIGHLIGHT 7 /**< Highlighted text style. */
#define CONSOLE_HIDDEN 8 /**< Hidden text style. */
#define CONSOLE_DEFAULT 0 /**< Default text style. */

#ifdef ICOV_CONSOLE_COLOR_ENABLED
#define CONSOLE_FORMAT(fmt) "\033[" STRINGIZE(fmt) "m"
#else
#define CONSOLE_FORMAT(fmt) ""
#endif

/**
 * @def CONSOLE_TODO
 * @brief Macro to format console text as a TODO message.
 *
 * This macro applies the formatting for a TODO message to the console text.
 */
#define CONSOLE_TODO                                                           \
  CONSOLE_FORMAT(CONSOLE_FG_BLUE) CONSOLE_FORMAT(CONSOLE_HIGHLIGHT)

/**
 * @brief Clears the console screen.
 *
 * This function clears the console screen by executing the appropriate system
 * command.
 */
inline void cls(void) {
  try {
    system("cls||clear");
  } catch (...) {
  }
  return;
}

} // namespace console
} // namespace icov
