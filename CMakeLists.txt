cmake_minimum_required(VERSION 3.10)

message("BUILD ICOV CORE PROJECT")

project(icov_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


set(ICOV_CORE_VERSION_MAJOR "0")
set(ICOV_CORE_VERSION_MINOR "5")
set(ICOV_CORE_VERSION_PATCH "0")
set(ICOV_CORE_VERSION "${ICOV_CORE_VERSION_MAJOR}.${ICOV_CORE_VERSION_MINOR}.${ICOV_CORE_VERSION_PATCH}")

file (WRITE "version.txt" ${ICOV_CORE_VERSION})

message(STATUS "[INFO] Building ICOV core version ${ICOV_CORE_VERSION}")

include_directories(src third-party/aff3ct/src third-party/aff3ct/include)

set(AFF3CT_COMPILE_EXE OFF)
set(AFF3CT_COMPILE_STATIC_LIB ON)
set(AFF3CT_POLAR_BIT_PACKING OFF)
set(AFF3CT_COLORS OFF)
add_subdirectory(third-party/aff3ct)

# Link with the "Threads library (required to link with AFF3CT after)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Link with AFF3CT
set (AFF3CT_DIR "${CMAKE_CURRENT_BINARY_DIR}/third-party/aff3ct/lib/cmake/aff3ct-2.3.5/")
find_package(AFF3CT CONFIG 2.3.5 REQUIRED)
message("AFF3CT_DIR=${AFF3CT_DIR}")

add_subdirectory(src)

if(ICOV_EXTENSIONS_ENABLED)
    add_subdirectory(extensions)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "[INFO] Building ICOV core tests")
    add_subdirectory(tests/unit_tests)
    add_subdirectory(tests/icov_interactive_coder_test)
    add_subdirectory(tests/icov_bit_symbol)
    add_subdirectory(tests/benchmarks)
endif()


message(STATUS "[INFO] Building ICOV core apps")
add_subdirectory(app/icov_imt_alist_generator)

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
