# Specify the minimum version of CMake required to build this project
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

#set(CMAKE_FIND_DEBUG_MODE TRUE)

# Define the ICOV core version number
set(ICOV_CORE_VERSION_MAJOR "1")
set(ICOV_CORE_VERSION_MINOR "9")
set(ICOV_CORE_VERSION_PATCH "0")
set(ICOV_CORE_VERSION
    "${ICOV_CORE_VERSION_MAJOR}.${ICOV_CORE_VERSION_MINOR}.${ICOV_CORE_VERSION_PATCH}"
)

# ##############################################################################
# INIT PROJECT              #
# ##############################################################################

# Specify the project name and the languages it is written in
project(icov_core LANGUAGES CXX C)

# Display a message indicating that the project is being built
message(STATUS "BUILD ICOV CORE PROJECT")

# ##############################################################################
# GLOBAL VARIABLES FOR ICOV       #
# ##############################################################################

# Define some global variables for the ICOV project - ICOV_BUILD_ENCODER: build
# the icov encoder library - ICOV_BUILD_EXTENSIONS: build extensions code for
# ICOV - ICOV_BUILD_APPS: build ICOV apps - ICOV_WITH_CPPCHECK: enable CPPCHECK
# - ICOV_WITH_CLANG_TIDY: enable Clang Tidy analysis -
# ICOV_USE_ADDRESS_SANITIZER: use sanitizer - ICOV_BYPASS_SETUP: in case you
# REALLY want to bypass the default setup. May cause severe issues. -

option(
  ICOV_BYPASS_SETUP
  "In case you REALLY want to bypass the default setup. May cause severe issues."
  OFF)

# ICOV_LOG_LEVEL: minimum log level
option(ICOV_LOG_BOOST "Build the icov log with boost" OFF)
option(ICOV_BUILD_ENCODER "Build the icov encoder lib" ON)
option(ICOV_BUILD_EXTENSIONS "Build extensions code for ICOV" ON)
option(ICOV_BUILD_APPS "Build ICOV apps" ON)
option(ICOV_WITH_CPPCHECK "ENABLE CPPCHECK" OFF)
option(ICOV_WITH_CLANG_TIDY "ENABLE CLANG TIDY ANALYSE" OFF)
option(ICOV_USE_ADDRESS_SANITIZER "Use Sanitizer" ON)
option(ICOV_ONLINE_MODE "Disable this option is you work offline" ON)
set(ICOV_LOG_LEVEL
    2
    CACHE STRING "Min Log Level")

# Define some flags for macros here - ICOV_CORE_DEBUG_TRACE_ENABLED: enable the
# trace debugger - ICOV_CORE_STACKTRACE_ENABLED: enable the stacktrace -
# ICOV_CHECK_BITPLANE_ENCODING_ENABLED: enable the bitplane decoding check right
# after decoding (use it to debug as it impacts the performances) -
# ICOV_CONSOLE_COLOR_ENABLED: enable color formatting of the console -
# ICOV_USE_OMP: enable parallelization with OMP - ICOV_ASSERTION_ENABLED: if set
# to OFF, some heavy assertions will be disabled to improve performances
option(ICOV_INTER_ENABLED "Enable inter (temporal) prediction in decoder" ON)
option(ICOV_CORE_DEBUG_TRACE_ENABLED "Enable the trace debugger" OFF)
option(ICOV_CORE_STACKTRACE_ENABLED "Enable the stacktrace" OFF)
option(ICOV_CHECK_BITPLANE_ENCODING_ENABLED
       "Enable the bitplane decoding check right after decoding\
    (use it to debug as it impacts the performances)" OFF)
option(ICOV_CONSOLE_COLOR_ENABLED "Enable color formatting of the console" OFF)
option(ICOV_USE_OMP "Enable parallelization with OMP" OFF)
option(
  ICOV_ASSERTION_ENABLED
  "If set to OFF, some heavy assertions will be disabled to improve performances"
  ON)

option(ICOV_ENCODER_EXPORT_TEXT_ENABLED
       "If enabled, ICOV exports textual informations at encoding" OFF)

# Set the ICOV_BUILD_TESTS variable to ON if it is not defined and the
# CMAKE_BUILD_TYPE matches Debug
if(NOT DEFINED ICOV_BUILD_TESTS AND CMAKE_BUILD_TYPE MATCHES Debug)
  option(ICOV_BUILD_TESTS "Force ICOV tests to be built. " ON)
else()
  option(ICOV_BUILD_TESTS "Force ICOV tests to be built. " OFF)
endif()

# Define whether to build shared libraries or not
option(BUILD_SHARED_LIBS "Build shared libs" OFF)

# Use folders in IDEs for projects (e.g. lib sample app test)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(ICOV_LOG_BOOST)
  add_compile_definitions(ICOV_LOG_BOOST)
endif()

if(NOT ICOV_ONLINE_MODE)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
               PROPERTY EP_UPDATE_DISCONNECTED 1)
endif()

# Include functions
message(VERBOSE "[TRACE] Load cmake includes")
include(ExternalProject)

if(NOT ICOV_BYPASS_SETUP)
  message(DEBUG "[DEBUG] Initial setup")

  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  # set(CMAKE_CXX_EXTENSIONS OFF)

  # use folders in IDEs for projects (e.g. lib sample app test)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
  link_directories(${LIBRARY_OUTPUT_PATH} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

  # some flags needed to build for windows
  if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /utf-8 /EHsc /MP")
  else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    add_compile_options(-mavx)
  endif()

  if(ICOV_USE_ADDRESS_SANITIZER AND CMAKE_BUILD_TYPE MATCHES Debug)
    # set address sanitizer compiler arguments
    # https://github.com/google/sanitizers/wiki/AddressSanitizer
    # message("=========== ID " ${CMAKE_CXX_COMPILER_ID}) if(
    # CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      # add compile options
      message(STATUS "[INFO] Use sanitizer")

      # -fsanitize=leak
      set(SANITIZE_OPTIONS
          -fsanitize=address
          -fsanitize=undefined
          -fno-sanitize-recover=all
          -fsanitize=float-divide-by-zero
          -fsanitize=float-cast-overflow
          -fno-sanitize=null
          -fno-sanitize=alignment)

      find_program(LLVM_SYMB_PATH NAMES llvm-symbolizer llvm-symbolizer-3.8
                                        llvm-symbolizer-3.7 llvm-symbolizer-3.6)

      if(LLVM_SYMB_PATH)
        message("[INFO] Use llvm-symbolizer from ${LLVM_SYMB_PATH}")
        set(ENV{ASAN_SYMBOLIZER_PATH} ${LLVM_SYMB_PATH})
      endif()

      set(ENV{LSAN_OPTIONS} LSAN_OPTIONS=symbolize=1)

      add_compile_options(${SANITIZE_OPTIONS})
      link_libraries(${SANITIZE_OPTIONS})

    endif()
    # if( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0 ) add_compile_options(
    # "-fabi-version=6" ) endif()
  endif()
else()
  message(WARNING "[WARNING] Initial setup is BYPASSED !")
endif()

# use ccache if available to compile faster https://ccache.dev/
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  message(STATUS "ccache found. using it.")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

# ##############################################################################
# INFOS COMPILER            #
# ##############################################################################
message(
  "-------------------------------------------------------------------------")
if(CMAKE_CXX_COMPILER_LOADED)
  message(STATUS "C++ Compiler path: ${CMAKE_CXX_COMPILER}")
  message(STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
  message(STATUS "C++ Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
  if(CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "C++ Compiler is part of GCC")
    execute_process(COMMAND "${CMAKE_CXX_COMPILER} -v" COMMAND_ECHO STDOUT)
  endif()
endif()
message("\n")

if(CMAKE_C_COMPILER_LOADED)
  message(STATUS "C Compiler path: ${CMAKE_C_COMPILER}")
  message(STATUS "C Compiler ID: ${CMAKE_C_COMPILER_ID}")
  message(STATUS "C Compiler version: ${CMAKE_C_COMPILER_VERSION}")
  if(CMAKE_COMPILER_IS_GNUC)
    message(STATUS "C Compiler is part of GCC")
  endif()
else()
  message(FATAL_ERROR "[ERROR] No C compiler defined")
endif()

message("\n")
message(STATUS "Cmake infos")
execute_process(COMMAND "${CMAKE_COMMAND}" --version COMMAND_ECHO STDOUT)
message(
  "-------------------------------------------------------------------------")
message(STATUS "[INFO] BIN OUTPUT PATH IS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "[INFO] LIB OUTPUT PATH IS ${LIBRARY_OUTPUT_PATH}")

# ##############################################################################
# CHECK DEPENDENCIES            #
# ##############################################################################
message(
  "-------------------------------------------------------------------------")
message(STATUS "Checking dependencies...\n"
)# cf https://cmake.org/cmake/help/latest/command/message.html
message(
  "-------------------------------------------------------------------------")

# ---------------------------------------------------------------------

# message("\n")

# ---------------------------------------------------------------------
# Link with the "Threads library (required to link with AFF3CT after)
message(STATUS "[INFO] Checking Threads support...")
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
message("\n")

# ---------------------------------------------------------------------
# LINK WITH OPENMP
if(ICOV_USE_OMP)
  message(STATUS "[INFO] Checking OpenMP support as ICOV_USE_OMP is ON...")
  find_package(OpenMP REQUIRED)
  message("\n")
endif()
# ---------------------------------------------------------------------
# Link with Boost
message(STATUS "[INFO] Checking Boost support...")
option(Boost_USE_STATIC_LIBS "Link Boost statically" ON)
set(Boost_USE_MULTITHREADED OFF)

# Removed because of issue on windows set(Boost_USE_STATIC_RUNTIME OFF)

# added this definition to make BOOST LOG working, see URL below in case of
# issue: https://github.com/boostorg/log/issues/46
add_definitions(-DBOOST_LOG_DYN_LINK)

find_package(
  Boost 1.70.0
  COMPONENTS program_options
             filesystem
             stacktrace_basic
             stacktrace_addr2line
             log
             log_setup
             system
             date_time
             thread
             chrono
             serialization
             json
  # OPTIONAL_COMPONENTS unit_test_framework stacktrace_backtrace stacktrace_noop
  REQUIRED)
include_directories(BEFORE SYSTEM ${Boost_INCLUDE_DIR})
message(STATUS "Include boost dir: ${Boost_INCLUDE_DIR}")
message("\n")

# ---------------------------------------------------------------------
# LINK WITH VVC

add_subdirectory("third-party/vvc/source/Lib/CommonLib")
add_subdirectory("third-party/vvc/source/Lib/Utilities")

# enable sse4.1 build for all source files for gcc and clang
if(UNIX OR MINGW)
  target_compile_options(CommonLib PUBLIC -w -Wno-register -msse4.1)
  add_compile_options("-msse4.1")
  add_compile_options("-fPIC")
endif()
# enable parallel build for Visual Studio
if(MSVC)
  add_compile_options("/MP")
  add_compile_options("/EHsc")
  target_compile_options(CommonLib PUBLIC "/wd4996")
endif()
include_directories(SYSTEM third-party/vvc/source/Lib)
set_target_properties(CommonLib PROPERTIES FOLDER "third-party")
message("\n")


# ---------------------------------------------------------------------
# Link with OpenCV
message(STATUS "[INFO] Checking OpenCV support...")
find_package(OpenCV
  PATH_SUFFIXES lib/cmake/opencv4
  REQUIRED)
set(ICOV_OPENCV_INCLUDE ${OpenCV_INCLUDE_DIRS})
set(ICOV_OPENCV_LIBS ${OpenCV_LIBS} ${OpenCV_LIBRARIES})
message(
  STATUS
    "[INFO] FOUND OpenCV: inc=${ICOV_OPENCV_INCLUDE} lib=${ICOV_OPENCV_LIBS}")
message("\n")
message(
  "-------------------------------------------------------------------------")

# ##############################################################################
# ICOV                   #
# ##############################################################################

# Set compile definitions based on the configuration options
add_compile_definitions("ICOV_LOG_LEVEL=${ICOV_LOG_LEVEL}")

# Enable debug trace if it is enabled in the configuration
if(ICOV_CORE_DEBUG_TRACE_ENABLED)
  message(STATUS "[INFO] ICOV core debug trace is ENABLED")
  add_compile_definitions("ICOV_CORE_DEBUG_TRACE_ENABLED")
endif()

# Enable stack trace if it is enabled in the configuration
if(ICOV_CORE_STACKTRACE_ENABLED)
  message(STATUS "[INFO] ICOV core stack trace is ENABLED")
  add_compile_definitions("ICOV_CORE_STACKTRACE_ENABLED"
                          "BOOST_STACKTRACE_USE_ADDR2LINE")
  # Check if dl libs is available, then link it
  if(CMAKE_DL_LIBS)
    message(DEBUG "Linking with ${CMAKE_DL_LIBS}")
    link_libraries(${CMAKE_DL_LIBS})
  endif()
endif()

# Enable bitplane check test if it is enabled in the configuration
if(ICOV_CHECK_BITPLANE_ENCODING_ENABLED)
  message(STATUS "[INFO] Encoded bitplane check is ENABLED")
  add_compile_definitions("ICOV_CHECK_BITPLANE_ENCODING_ENABLED")
endif()

# Enable textual debug for encoder if it is enabled in the configuration
if(ICOV_ENCODER_EXPORT_TEXT_ENABLED)
  message(STATUS "[INFO] ICOV encoder will ewport textual debug infos")
  add_compile_definitions("ICOV_ENCODER_EXPORT_TEXT")
endif()

# Enable colored console output if it is enabled in the configuration
if(ICOV_CONSOLE_COLOR_ENABLED)
  message(STATUS "[INFO] Colored log in console are ENABLED")
  add_compile_definitions("ICOV_CONSOLE_COLOR_ENABLED")
endif()

# Enable assertions if it is enabled in the configuration
if(ICOV_ASSERTION_ENABLED)
  message(STATUS "[INFO] Assertions are ENABLED")
  add_compile_definitions("ICOV_ASSERTION_ENABLED")
endif()

if(NOT ICOV_INTER_ENABLED)
  message(STATUS "[INFO] INTER prediction is DISABLED")
  add_compile_definitions("ICOV_NO_INTER")
endif()

# Write the version number to a file
file(WRITE "version.txt" ${ICOV_CORE_VERSION})

# Write the version number to a header file
file(
  WRITE "${CMAKE_CURRENT_SOURCE_DIR}/lib/icov/core/common/version.h"
  "/* This file is auto-generated from Cmake. You should not modify it directly*/\n"
  "#pragma once\n"
  "#define ICOV_CORE_VERSION_MAJOR ${ICOV_CORE_VERSION_MAJOR}\n"
  "#define ICOV_CORE_VERSION_MINOR ${ICOV_CORE_VERSION_MINOR}\n"
  "#define ICOV_CORE_VERSION_PATCH ${ICOV_CORE_VERSION_PATCH}\n"
  "#define ICOV_CORE_VERSION \"${ICOV_CORE_VERSION}\"")

# Output the version number to console
message(STATUS "[INFO] Building ICOV core version ${ICOV_CORE_VERSION}")

# Add the required directories to the include source files
include_directories(. lib lib/icov)

# Add subdirectories to build
add_subdirectory(lib/icov)
add_subdirectory(lib/extractor)
add_subdirectory(lib/decoder)

# Check if encoder should be built, then add its subdirectory
if(ICOV_BUILD_ENCODER)
  add_subdirectory(lib/encoder)
endif()

# Check if extensions should be built, then add its subdirectory
if(ICOV_BUILD_EXTENSIONS)
  add_subdirectory(lib/extensions)
endif()

# ##############################################################################
# ADD EXECUTABLES            #
# ##############################################################################

# If ICOV_BUILD_TESTS is set to true, build the unit tests and benchmarks
if(ICOV_BUILD_TESTS)
  message(
    "-------------------------------------------------------------------------")
  message(STATUS "[INFO] Building ICOV core tests")
  message(
    "-------------------------------------------------------------------------")
  enable_testing()
  add_subdirectory(tests/unit_tests)
  add_subdirectory(tests/benchmarks)
  if(ICOV_BUILD_APPS)
    add_subdirectory(tests/integration)
  endif()
endif()

# If ICOV_BUILD_APPS is set to true, build the executables
if(ICOV_BUILD_APPS)
  message(
    "-------------------------------------------------------------------------")
  message(STATUS "[INFO] Building ICOV core executables")
  message(
    "-------------------------------------------------------------------------")
  add_subdirectory(lib/app_common)
  add_subdirectory(app)
endif(ICOV_BUILD_APPS)

# INSTALL DEMO
if(INSTALL_DEMO)
  message(STATUS "[INFO] Configuring ICOV demo")
  set(ICOV_DEMO_ROOT_DIR "${CMAKE_BINARY_DIR}/demo")

  make_directory(${ICOV_DEMO_ROOT_DIR})

  if(NOT DEFINED ICOV_DEMO_VERSION)
    set(ICOV_DEMO_VERSION_PATH ${ICOV_DEMO_ROOT_DIR}/version.txt)
    file(
      DOWNLOAD
      https://sourceforge.net/projects/icov/files/icov-demonstrator/version.txt
      ${ICOV_DEMO_VERSION_PATH})
    file(STRINGS "${ICOV_DEMO_VERSION_PATH}" ICOV_DEMO_VERSION)
  endif()

  set(ICOV_DEMO_DIR "${ICOV_DEMO_ROOT_DIR}/${ICOV_DEMO_VERSION}")

  # write version
  make_directory(${ICOV_DEMO_DIR})

  # if(UNIX) message("DLunix") endif()

  message(
    STATUS "[INFO] Downloading demo v${ICOV_DEMO_VERSION} to ${ICOV_DEMO_DIR}")
  file(
    DOWNLOAD
    https://downloads.sourceforge.net/project/icov/icov-demonstrator/icov-equirect-sample/v1/linux/demo-linux.zip
    ${ICOV_DEMO_ROOT_DIR}/demo.zip)

  message(STATUS "[INFO] Extracting demo")
  file(ARCHIVE_EXTRACT INPUT ${ICOV_DEMO_ROOT_DIR}/demo.zip DESTINATION
       ${ICOV_DEMO_DIR} VERBOSE)
endif()

file(COPY "data/" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data")
