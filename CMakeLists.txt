cmake_minimum_required(VERSION 3.10)

message("BUILD ICOV CORE PROJECT")

project(icov_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


set(ICOV_CORE_VERSION_MAJOR "0")
set(ICOV_CORE_VERSION_MINOR "6")
set(ICOV_CORE_VERSION_PATCH "0")
set(ICOV_CORE_VERSION "${ICOV_CORE_VERSION_MAJOR}.${ICOV_CORE_VERSION_MINOR}.${ICOV_CORE_VERSION_PATCH}")

option(ICOV_CORE_DEBUG_TRACE_ENABLED "Enable the trace debugger" OFF)

if(ICOV_CORE_DEBUG_TRACE_ENABLED)
    message(STATUS "[INFO] ICOV CORE DEBUG TRACING IS ENABLED")
    add_compile_definitions("ICOV_CORE_DEBUG_TRACE_ENABLED")
endif()

file (WRITE "version.txt" ${ICOV_CORE_VERSION})

message(STATUS "[INFO] Building ICOV core version ${ICOV_CORE_VERSION}")

include_directories(src third-party/aff3ct/src third-party/aff3ct/include third-party/aff3ct/lib/MIPP/src third-party/aff3ct/lib/cli/src
    third-party/aff3ct/lib/rang/include)

#Link with Boost

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.70.0 COMPONENTS
    program_options
    filesystem
    REQUIRED)

# Link with the "Threads library (required to link with AFF3CT after)

#set(CMAKE_THREAD_PREFER_PTHREAD ON)
#set(THREADS_PREFER_PTHREAD_FLAG ON)
#find_package(Threads REQUIRED)

# Link with AFF3CT

set(AFF3CT_COMPILE_EXE OFF)
set(AFF3CT_COMPILE_STATIC_LIB ON)
set(AFF3CT_POLAR_BIT_PACKING OFF)
set(AFF3CT_COLORS OFF)

#set(AFF3CT_VERSION_FULL CACHE STRING "aaa" FORCE)
#mark_as_advanced(AFF3CT_VERSION_FULL)
#message("AFF VERSION ${AFF3CT_VERSION_FULL}")
add_subdirectory(third-party/aff3ct)

message("build ${AFF3CT_VERSION_FULL}")
set (AFF3CT_DIR "${CMAKE_CURRENT_BINARY_DIR}/third-party/aff3ct/lib/cmake/")
message("SEARCHING AFF3CT IN AFF3CT_DIR=${AFF3CT_DIR}")


#SET(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
#SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)

#find_package(AFF3CT CONFIG 2.3.5 REQUIRED PATHS "${CMAKE_CURRENT_BINARY_DIR}/third-party/aff3ct/lib/cmake/" NO_DEFAULT_PATH)
#message(STATUS "FOUND AFF3CT IN AFF3CT_CONFIG=${AFF3CT_CONFIG}")

add_subdirectory(src)

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
add_subdirectory(app/icov_cmd_app)

#set(INSTALL_DEMO 1)

# message(${CMAKE_CURRENT_SOURCE_DIR})
# message(${CMAKE_SOURCE_DIR})
# message(${CMAKE_BINARY_DIR})
# message(${EXECUTABLE_OUTPUT_PATH})

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

FILE(COPY "data/" DESTINATION "${EXECUTABLE_OUTPUT_PATH}/data")
