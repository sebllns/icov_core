cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

########################################
#            INIT PROJECT              #
########################################
project(icov_core LANGUAGES CXX)
message(STATUS "BUILD ICOV CORE PROJECT")

########################################
#      GLOBAL VARIABLES FOR ICOV       #
########################################
set(ICOV_BUILD_ENCODER ON CACHE BOOL "Build the icov encoder lib")
set(ICOV_BUILD_EXTENSIONS ON CACHE BOOL "Build extensions code for ICOV")
set(ICOV_BUILD_APPS ON CACHE BOOL "Build ICOV apps")
set(ICOV_WITH_CPPCHECK OFF CACHE BOOL "ENABLE CPPCHECK")
set(ICOV_WITH_CLANG_TIDY OFF CACHE BOOL "ENABLE CLANG TIDY ANALYSE")
set(ICOV_USE_ADDRESS_SANITIZER ON CACHE BOOL "Use Sanitizer")
set(ICOV_BYPASS_SETUP OFF CACHE BOOL "In case you REALLY want to bypass the default setup. May cause severe issues.")
set(ICOV_LOG_LEVEL 2 CACHE STRING "Min Log Level")

#define some flags for macros here
option(ICOV_CORE_DEBUG_TRACE_ENABLED "Enable the trace debugger" OFF)
option(ICOV_CORE_STACKTRACE_ENABLED "Enable the stacktrace" OFF)
option(ICOV_CHECK_BITPLANE_ENCODING_ENABLED "Enable the bitplane decoding check right after decoding\
    (use it to debug as it impacts the performances)" OFF)
option(ICOV_CONSOLE_COLOR_ENABLED "Enable color formatting of the console" OFF)
option(ICOV_USE_OMP "Enable parallelization with OMP" OFF)

if(NOT DEFINED ICOV_BUILD_TESTS AND CMAKE_BUILD_TYPE MATCHES Debug)
    set(ICOV_BUILD_TESTS ON CACHE BOOL "Force ICOV tests to be built. ")
else()
     set(ICOV_BUILD_TESTS OFF CACHE BOOL "Force ICOV tests to be built. ")
endif()

# use folders in IDEs for projects (e.g. lib sample app test)
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

#Include functions
message(VERBOSE "Includes")
include(${CMAKE_CURRENT_SOURCE_DIR}/.cmake/AddGitSubmodule.cmake)
include(ExternalProject)

if(NOT ICOV_BYPASS_SETUP)
 message(DEBUG "Initial setup")
#some flags needed to build for windows
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /utf-8 /EHsc /MP")
endif()
if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

# set address sanitizer compiler arguments
# https://github.com/google/sanitizers/wiki/AddressSanitizer
if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
        if(ICOV_USE_ADDRESS_SANITIZER)
            # add compile options
            message(STATUS "Use sanitizer")

            SET(SANITIZE_OPTIONS -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment)

            add_compile_options(${SANITIZE_OPTIONS})
            link_libraries(${SANITIZE_OPTIONS})


#            add_compile_options( "-fsanitize=address" )
#            add_link_options( "-fsanitize=address" )
        endif()
    endif()
    if( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0 )
        add_compile_options( "-fabi-version=6" )
    endif()
else()
    message(WARNING "Initial setup is BYPASSED !")
endif()

# use ccache if available to compile faster
# https://ccache.dev/
find_program( CCACHE_FOUND ccache )
if( CCACHE_FOUND )
    message( STATUS "ccache found. using it." )
    set_property( GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache )
    set_property( GLOBAL PROPERTY RULE_LAUNCH_LINK ccache )
endif()
endif()

########################################
#            INFOS COMPILER            #
########################################
message("-------------------------------------------------------------------------")
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
    message(STATUS "No C compiler defined")
endif()

message("\n")
message(STATUS "Cmake infos")
execute_process(COMMAND "${CMAKE_COMMAND}" --version COMMAND_ECHO STDOUT)
message("-------------------------------------------------------------------------")

########################################
#        CHECK DEPENDENCIES            #
########################################
message(STATUS "Checking dependencies...\n") #cf https://cmake.org/cmake/help/latest/command/message.html

#---------------------------------------------------------------------
# Use Conan
#message(STATUS "Using conan...")
#include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
#conan_basic_setup()

#message("\n")

#---------------------------------------------------------------------
# Link with the "Threads library (required to link with AFF3CT after)
message(STATUS "[INFO] Checking Threads support...")
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
message("\n")

#---------------------------------------------------------------------
#LINK WITH OPENMP
if(ICOV_USE_OMP)
message(STATUS "[INFO] Checking OpenMP support as ICOV_USE_OMP is ON...")
find_package(OpenMP REQUIRED)
message("\n")
endif()
#---------------------------------------------------------------------
#Link with Boost
message(STATUS "[INFO] Checking Boost support...")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)

# Removed because of issue on windows
#set(Boost_USE_STATIC_RUNTIME OFF)

#added this definition to make BOOST LOG working, see URL below in case of issue:
#https://github.com/boostorg/log/issues/46
add_definitions(-DBOOST_LOG_DYN_LINK)

find_package(Boost 1.70.0 
    COMPONENTS
    program_options
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
#    OPTIONAL_COMPONENTS
#    unit_test_framework
#stacktrace_backtrace
#stacktrace_noop
    REQUIRED)
include_directories(BEFORE SYSTEM ${Boost_INCLUDE_DIR})
message(STATUS "Include boost dir: ${Boost_INCLUDE_DIR}")
message("\n")

#---------------------------------------------------------------------
#LINK WITH VVC

add_subdirectory("third-party/vvc/source/Lib/CommonLib")
add_subdirectory("third-party/vvc/source/Lib/Utilities")

# enable sse4.1 build for all source files for gcc and clang
if( UNIX OR MINGW )
    target_compile_options(CommonLib PUBLIC -w -Wno-register -msse4.1)
    add_compile_options( "-msse4.1" )
endif()
# enable parallel build for Visual Studio
if( MSVC )
    add_compile_options( "/MP" )
    add_compile_options( "/EHsc" )
    target_compile_options(CommonLib PUBLIC "/wd4996")
endif()
include_directories(SYSTEM third-party/vvc/source/Lib)
set_target_properties(CommonLib PROPERTIES FOLDER "third-party" )
message("\n")


#---------------------------------------------------------------------
# Link with AFF3CT
message(STATUS "[INFO] Checking AFF3CT support...")
set(AFF3CT_BUILD_DIR third-party/aff3ct CACHE STRING "Path to aff3ct build directory default is ${CMAKE_BINARY_DIR}/third-party/aff3ct")
set(AFF3CT_BUILD_DIR_FULL ${CMAKE_BINARY_DIR}/${AFF3CT_BUILD_DIR})
include(${CMAKE_CURRENT_SOURCE_DIR}/.cmake/install_aff3ct.cmake)

set (AFF3CT_PACKAGE_DIR "${AFF3CT_BUILD_DIR_FULL}/lib/cmake/")
message(STATUS "[INFO] SEARCHING AFF3CT IN ${AFF3CT_PACKAGE_DIR}")
find_package(AFF3CT CONFIG 2.3.5 REQUIRED PATHS "${AFF3CT_PACKAGE_DIR}" NO_DEFAULT_PATH)
message(STATUS "[INFO] FOUND AFF3CT: ${AFF3CT_CONFIG}")

#include_directories(SYSTEM third-party/aff3ct/src third-party/aff3ct/include third-party/aff3ct/lib/MIPP/src third-party/aff3ct/lib/cli/src
#    third-party/aff3ct/lib/rang/include)

message(WARNING "Move aff3ct to better place (lib/icov/ldpc_codec_layer)")
message("\n")
message("-------------------------------------------------------------------------")

########################################
#               ICOV                   #
########################################
set(ICOV_CORE_VERSION_MAJOR "1")
set(ICOV_CORE_VERSION_MINOR "0")
set(ICOV_CORE_VERSION_PATCH "1")
set(ICOV_CORE_VERSION "${ICOV_CORE_VERSION_MAJOR}.${ICOV_CORE_VERSION_MINOR}.${ICOV_CORE_VERSION_PATCH}")

add_compile_definitions("ICOV_LOG_LEVEL=${ICOV_LOG_LEVEL}")

if(ICOV_CORE_DEBUG_TRACE_ENABLED)
    message(STATUS "[INFO] ICOV core debug trace is ENABLED")
    add_compile_definitions("ICOV_CORE_DEBUG_TRACE_ENABLED")
endif()

if(ICOV_CORE_STACKTRACE_ENABLED)
    message(STATUS "[INFO] ICOV core stack trace is ENABLED")
    add_compile_definitions("ICOV_CORE_STACKTRACE_ENABLED" "BOOST_STACKTRACE_USE_ADDR2LINE")
    if(CMAKE_DL_LIBS)
        message(DEBUG "Linking with ${CMAKE_DL_LIBS}")
        link_libraries(${CMAKE_DL_LIBS})
    endif()
endif()

if(ICOV_CHECK_BITPLANE_ENCODING_ENABLED)
    add_compile_definitions("ICOV_CHECK_BITPLANE_ENCODING_ENABLED")
endif()

if(ICOV_CONSOLE_COLOR_ENABLED)
    add_compile_definitions("ICOV_CONSOLE_COLOR_ENABLED")
endif()

file (WRITE "version.txt" ${ICOV_CORE_VERSION})
file (WRITE "${CMAKE_CURRENT_SOURCE_DIR}/lib/icov/core/common/version.h"
    "/* This file is auto-generated from Cmake. You should not modify it directly*/\n"
    "#pragma once\n"
    "#define ICOV_CORE_VERSION_MAJOR ${ICOV_CORE_VERSION_MAJOR}\n"
    "#define ICOV_CORE_VERSION_MINOR ${ICOV_CORE_VERSION_MINOR}\n"
    "#define ICOV_CORE_VERSION_PATCH ${ICOV_CORE_VERSION_PATCH}\n"
    "#define ICOV_CORE_VERSION \"${ICOV_CORE_VERSION}\"")
message(STATUS "[INFO] Building ICOV core version ${ICOV_CORE_VERSION}")

#REQUIRED
include_directories(. lib lib/icov)
add_subdirectory(lib/icov)

if(ICOV_BUILD_ENCODER)
    add_subdirectory(lib/encoder)
endif()

add_subdirectory(lib/decoder)

if(ICOV_BUILD_EXTENSIONS)
    add_subdirectory(lib/extensions)
endif()

if(ICOV_BUILD_TESTS)
    message(STATUS "[INFO] Building ICOV core tests")
    add_subdirectory(tests/unit_tests)
    add_subdirectory(tests/benchmarks)
endif()

if(ICOV_BUILD_APPS)
    add_subdirectory(app)
endif(ICOV_BUILD_APPS)

#INSTALL DEMO
if(INSTALL_DEMO)
    message(STATUS "[INFO] Configuring ICOV demo")
    set(ICOV_DEMO_ROOT_DIR "${CMAKE_BINARY_DIR}/demo")

    make_directory(${ICOV_DEMO_ROOT_DIR})

    if(NOT DEFINED ICOV_DEMO_VERSION)
        set(ICOV_DEMO_VERSION_PATH ${ICOV_DEMO_ROOT_DIR}/version.txt)
        file(DOWNLOAD https://sourceforge.net/projects/icov/files/icov-demonstrator/version.txt ${ICOV_DEMO_VERSION_PATH})
        file(STRINGS "${ICOV_DEMO_VERSION_PATH}" ICOV_DEMO_VERSION)
    endif()

    set(ICOV_DEMO_DIR "${ICOV_DEMO_ROOT_DIR}/${ICOV_DEMO_VERSION}")

    #write version
    make_directory(${ICOV_DEMO_DIR})

    #     if(UNIX)
    # message("DLunix")
    # endif()

    message(STATUS "[INFO] Downloading demo v${ICOV_DEMO_VERSION} to ${ICOV_DEMO_DIR}")
    file(DOWNLOAD https://downloads.sourceforge.net/project/icov/icov-demonstrator/icov-equirect-sample/v1/linux/demo-linux.zip  ${ICOV_DEMO_ROOT_DIR}/demo.zip)

    message(STATUS "[INFO] Extracting demo")
    file(ARCHIVE_EXTRACT INPUT ${ICOV_DEMO_ROOT_DIR}/demo.zip DESTINATION ${ICOV_DEMO_DIR} VERBOSE)
endif()

FILE(COPY "data/" DESTINATION "${EXECUTABLE_OUTPUT_PATH}/data")


