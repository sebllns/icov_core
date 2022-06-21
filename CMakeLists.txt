cmake_minimum_required(VERSION 3.2)

project(icov_core LANGUAGES CXX)

message("BUILD ICOV CORE PROJECT")

include(${CMAKE_CURRENT_SOURCE_DIR}/.cmake/AddGitSubmodule.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

include(ExternalProject)
# set address sanitizer compiler arguments
if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
  if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
    if( USE_ADDRESS_SANITIZER )
      # add compile options
      add_compile_options( "-fsanitize=address" )
      add_link_options( "-fsanitize=address" )
    endif()
  endif()
  if( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0 )
    add_compile_options( "-fabi-version=6" )
  endif()
endif()

#Link with Boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.70.0 COMPONENTS
    program_options
    filesystem
    REQUIRED)
#END BOOST

# Link with the "Threads library (required to link with AFF3CT after)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
#END THREADS

# Link with AFF3CT
#SET(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
#SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)

set(AFF3CT_BUILD_DIR third-party/aff3ct CACHE STRING "Path to aff3ct build directory default is ${CMAKE_BINARY_DIR}/third-party/aff3ct")
set(AFF3CT_BUILD_DIR_FULL ${CMAKE_BINARY_DIR}/${AFF3CT_BUILD_DIR})
include(${CMAKE_CURRENT_SOURCE_DIR}/.cmake/install_aff3ct.cmake)

set (AFF3CT_PACKAGE_DIR "${AFF3CT_BUILD_DIR_FULL}/lib/cmake/")
message(STATUS "[INFO] SEARCHING AFF3CT IN ${AFF3CT_PACKAGE_DIR}")
find_package(AFF3CT CONFIG 2.3.5 REQUIRED PATHS "${AFF3CT_PACKAGE_DIR}" NO_DEFAULT_PATH)
message(STATUS "[INFO] FOUND AFF3CT: ${AFF3CT_CONFIG}")
include_directories(SYSTEM third-party/aff3ct/src third-party/aff3ct/include third-party/aff3ct/lib/MIPP/src third-party/aff3ct/lib/cli/src
    third-party/aff3ct/lib/rang/include)
#END AFF3CT

#LINK WITH VVC
add_subdirectory("third-party/vvc/source/Lib/CommonLib")
target_compile_options(CommonLib PUBLIC "-w" "-Wno-register")
# enable sse4.1 build for all source files for gcc and clang
if( UNIX OR MINGW )
  add_compile_options( "-msse4.1" )
endif()
# enable parallel build for Visual Studio
if( MSVC )
  add_compile_options( "/MP" )
  add_compile_options( "/EHsc" )
endif()
include_directories(SYSTEM third-party/vvc/source/Lib)
#END VCC

#ICOV PROJECT
set(ICOV_CORE_VERSION_MAJOR "0")
set(ICOV_CORE_VERSION_MINOR "8")
set(ICOV_CORE_VERSION_PATCH "0")
set(ICOV_CORE_VERSION "${ICOV_CORE_VERSION_MAJOR}.${ICOV_CORE_VERSION_MINOR}.${ICOV_CORE_VERSION_PATCH}")


option(ICOV_CORE_DEBUG_TRACE_ENABLED "Enable the trace debugger" OFF)

if(ICOV_CORE_DEBUG_TRACE_ENABLED)
    message(STATUS "[INFO] ICOV CORE DEBUG TRACING IS ENABLED")
    add_compile_definitions("ICOV_CORE_DEBUG_TRACE_ENABLED")
endif()

file (WRITE "version.txt" ${ICOV_CORE_VERSION})

message(STATUS "[INFO] Building ICOV core version ${ICOV_CORE_VERSION}")

add_subdirectory(src)
include_directories(. src)

if(ICOV_EXTENSIONS_ENABLED)
    add_subdirectory(extensions)
endif()

add_subdirectory(encoder)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "[INFO] Building ICOV core tests")
    add_subdirectory(tests/unit_tests)
    add_subdirectory(tests/benchmarks)
endif()

add_subdirectory(tests/icov_interactive_coder_bit_symbol)

message(STATUS "[INFO] Building ICOV core apps")
add_subdirectory(app/cli)
if(ICOV_PLATFORM MATCHES LINUX OR ICOV_PLATFORM MATCHES WIN)
    add_subdirectory(app/opencv_interactive_viewport)
endif()
#END ICOV

#DEMO
if(INSTALL_DEMO)

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
#END DEMO

#RESOURCES
FILE(COPY "data/" DESTINATION "${EXECUTABLE_OUTPUT_PATH}/data")
#END RESOURCES

#PRINT DEBUG INFO
execute_process(COMMAND "${CMAKE_CXX_COMPILER} -v" COMMAND_ECHO STDOUT)
execute_process(COMMAND "${CMAKE_COMMAND} -v" COMMAND_ECHO STDOUT)
#END PRINT

