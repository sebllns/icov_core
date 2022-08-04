cmake_minimum_required(VERSION 3.2)

########################################
#            INIT PROJECT              #
########################################
project(icov_core LANGUAGES CXX)
message(STATUS "BUILD ICOV CORE PROJECT")

set(BUILD_ICOV_EXTENSIONS ON CACHE BOOL "Build extensions code for ICOV")
set(BUILD_ICOV_ENCODER ON CACHE BOOL "Build the icov encoder lib")
set(BUILD_ICOV_APPS ON CACHE BOOL "Build ICOV apps")
set(WITH_CPPCHECK OFF CACHE BOOL "ENABLE CPPCHECK")
set(WITH_CLANG_TIDY OFF CACHE BOOL "ENABLE CLANG TIDY ANALYSE")

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
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /utf-8 /EHsc /MP")
endif()

if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

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
# Link with the "Threads library (required to link with AFF3CT after)
message(STATUS "[INFO] Checking Threads support...")
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
message("\n")

#---------------------------------------------------------------------

#Link with Boost
message(STATUS "[INFO] Checking Boost support...")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
#set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.70.0 
    COMPONENTS
    program_options
    filesystem
#    OPTIONAL_COMPONENTS
#    unit_test_framework
    REQUIRED)
include_directories(BEFORE SYSTEM ${Boost_INCLUDE_DIR})
message(STATUS "Include boost dir: ${Boost_INCLUDE_DIR}")
message("\n")

#---------------------------------------------------------------------
#LINK WITH VVC
# use ccache
find_program( CCACHE_FOUND ccache )
if( CCACHE_FOUND )
    message( STATUS "ccache found. using it." )
    set_property( GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache )
    set_property( GLOBAL PROPERTY RULE_LAUNCH_LINK ccache )
endif()

add_subdirectory("third-party/vvc/source/Lib/CommonLib")
add_subdirectory("third-party/vvc/source/Lib/Utilities")

# enable sse4.1 build for all source files for gcc and clang
if( UNIX OR MINGW )
    target_compile_options(CommonLib PUBLIC "-w" "-Wno-register")
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
set(ICOV_CORE_VERSION_MAJOR "0")
set(ICOV_CORE_VERSION_MINOR "8")
set(ICOV_CORE_VERSION_PATCH "6")
set(ICOV_CORE_VERSION "${ICOV_CORE_VERSION_MAJOR}.${ICOV_CORE_VERSION_MINOR}.${ICOV_CORE_VERSION_PATCH}")
option(ICOV_CORE_DEBUG_TRACE_ENABLED "Enable the trace debugger" OFF)

if(ICOV_CORE_DEBUG_TRACE_ENABLED)
    message(STATUS "[INFO] ICOV core debug trace is ENABLED")
    add_compile_definitions("ICOV_CORE_DEBUG_TRACE_ENABLED")
endif()

file (WRITE "version.txt" ${ICOV_CORE_VERSION})
message(STATUS "[INFO] Building ICOV core version ${ICOV_CORE_VERSION}")

#REQUIRED
include_directories(. lib lib/icov)
add_subdirectory(lib/icov)

if(BUILD_ICOV_ENCODER)
    add_subdirectory(lib/encoder)
endif()

if(BUILD_ICOV_EXTENSIONS)
    add_subdirectory(lib/extensions)
endif()

if(BUILD_ICOV_TESTS OR (NOT DEFINED BUILD_ICOV_TESTS AND CMAKE_BUILD_TYPE MATCHES Debug))
    message(STATUS "[INFO] Building ICOV core tests")
    add_subdirectory(tests/unit_tests)
    add_subdirectory(tests/benchmarks)
endif()

if(BUILD_ICOV_APPS)
    add_subdirectory(app)
endif(BUILD_ICOV_APPS)

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


