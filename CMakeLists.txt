cmake_minimum_required(VERSION 3.2)

########################################
#            INIT PROJECT              #
########################################
project(icov_core LANGUAGES CXX)
message(STATUS "BUILD ICOV CORE PROJECT")

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
#Link with Boost
message(STATUS "[INFO] Checking Boost support...")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.70.0 
COMPONENTS
    program_options
    filesystem
OPTIONAL_COMPONENTS
    unit_test_framework
REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
message(STATUS "Include boost dir: ${Boost_INCLUDE_DIRS}")
message("\n")

#---------------------------------------------------------------------
# Link with the "Threads library (required to link with AFF3CT after)
message(STATUS "[INFO] Checking Threads support...")
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
message("\n")

#---------------------------------------------------------------------
# Link with AFF3CT
message(STATUS "[INFO] Checking AFF3CT support...")
#SET(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
#SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)

set(AFF3CT_BUILD_DIR third-party/aff3ct CACHE STRING "Path to aff3ct build directory default is ${CMAKE_BINARY_DIR}/third-party/aff3ct")
set(AFF3CT_BUILD_DIR_FULL ${CMAKE_BINARY_DIR}/${AFF3CT_BUILD_DIR})
include(${CMAKE_CURRENT_SOURCE_DIR}/.cmake/install_aff3ct.cmake)

set (AFF3CT_PACKAGE_DIR "${AFF3CT_BUILD_DIR_FULL}/lib/cmake/")
message(STATUS "[INFO] SEARCHING AFF3CT IN ${AFF3CT_PACKAGE_DIR}")
find_package(AFF3CT CONFIG 2.3.5 REQUIRED PATHS "${AFF3CT_PACKAGE_DIR}" NO_DEFAULT_PATH)
message(STATUS "[INFO] FOUND AFF3CT: ${AFF3CT_CONFIG}")

# set(AFF3CT_COMPILE_EXE OFF)
# set(AFF3CT_COMPILE_STATIC_LIB ON)
# set(AFF3CT_POLAR_BIT_PACKING OFF)
# set(AFF3CT_COLORS OFF)

# set(aff3ct_PREFIX "${CMAKE_BINARY_DIR}/third-party/aff3ct")
# set(aff3ct_INSTALL_DIR "${aff3ct_PREFIX}")
# set(aff3ct_CMAKE_ARGS 
    # -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS}"
    # -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE 
    # -DCMAKE_INSTALL_PREFIX=${aff3ct_INSTALL_DIR}
    # -DAFF3CT_COMPILE_EXE=${AFF3CT_COMPILE_EXE}
    # -DAFF3CT_COMPILE_STATIC_LIB=${AFF3CT_COMPILE_STATIC_LIB}
    # -DAFF3CT_POLAR_BIT_PACKING=${AFF3CT_POLAR_BIT_PACKING}
    # -DAFF3CT_COLORS=${AFF3CT_COLORS})

# message("aff3ct_PREFIX='${aff3ct_PREFIX}'")
# message("aff3ct_INSTALL_DIR='${aff3ct_INSTALL_DIR}'")
# message("aff3ct_CMAKE_ARGS='${aff3ct_CMAKE_ARGS}'")
# ExternalProject_Add(aff3ct_project
    # PREFIX ${aff3ct_PREFIX}
    # GIT_REPOSITORY https://github.com/aff3ct/aff3ct.git
    # GIT_SUBMODULES_RECURSE TRUE
    # GIT_PROGRESS TRUE
    # INSTALL_DIR ${aff3ct_INSTALL_DIR}
    # CMAKE_ARGS ${aff3ct_CMAKE_ARGS})

include_directories(third-party/aff3ct/src third-party/aff3ct/include third-party/aff3ct/lib/MIPP/src third-party/aff3ct/lib/cli/src
    third-party/aff3ct/lib/rang/include)
message("-------------------------------------------------------------------------")

########################################
#               ICOV                   #
########################################
set(ICOV_CORE_VERSION_MAJOR "0")
set(ICOV_CORE_VERSION_MINOR "8")
set(ICOV_CORE_VERSION_PATCH "0")
set(ICOV_CORE_VERSION "${ICOV_CORE_VERSION_MAJOR}.${ICOV_CORE_VERSION_MINOR}.${ICOV_CORE_VERSION_PATCH}")
option(ICOV_CORE_DEBUG_TRACE_ENABLED "Enable the trace debugger" OFF)

if(ICOV_CORE_DEBUG_TRACE_ENABLED)
    message(STATUS "[INFO] ICOV core debug trace is ENABLED")
    add_compile_definitions("ICOV_CORE_DEBUG_TRACE_ENABLED")
endif()

file (WRITE "version.txt" ${ICOV_CORE_VERSION})

message(STATUS "[INFO] Building ICOV core version ${ICOV_CORE_VERSION}")

include_directories(. src)
add_subdirectory(src)

if(ICOV_EXTENSIONS_ENABLED)
    #add_subdirectory(extensions)
endif()

#add_subdirectory(encoder)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "[INFO] Building ICOV core tests")
    #add_subdirectory(tests/unit_tests)
    #add_subdirectory(tests/benchmarks)
endif()

#add_subdirectory(tests/icov_interactive_coder_bit_symbol)

message(STATUS "[INFO] Building ICOV core apps")
#add_subdirectory(app/cli)
if(ICOV_PLATFORM MATCHES LINUX OR ICOV_PLATFORM MATCHES WIN)
    #add_subdirectory(app/opencv_interactive_viewport)
endif()

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

# if(UNIX)
# message("DLunix")
# endif()

    message(STATUS "[INFO] Downloading demo v${ICOV_DEMO_VERSION} to ${ICOV_DEMO_DIR}")
    file(DOWNLOAD https://downloads.sourceforge.net/project/icov/icov-demonstrator/icov-equirect-sample/v1/linux/demo-linux.zip  ${ICOV_DEMO_ROOT_DIR}/demo.zip)

    message(STATUS "[INFO] Extracting demo")
    file(ARCHIVE_EXTRACT INPUT ${ICOV_DEMO_ROOT_DIR}/demo.zip DESTINATION ${ICOV_DEMO_DIR} VERBOSE)
endif()

FILE(COPY "data/" DESTINATION "${EXECUTABLE_OUTPUT_PATH}/data")



