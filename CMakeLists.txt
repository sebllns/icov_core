cmake_minimum_required(VERSION 3.10)

message("BUILD ICOV CORE PROJECT")

project(icov_core LANGUAGES CXX)


set(ICOV_CORE_VERSION_MAJOR "0")
set(ICOV_CORE_VERSION_MINOR "2")
set(ICOV_CORE_VERSION_PATCH "1")
set(ICOV_CORE_VERSION "${ICOV_CORE_VERSION_MAJOR}.${ICOV_CORE_VERSION_MINOR}.${ICOV_CORE_VERSION_PATCH}")

file (WRITE "version.txt" ${ICOV_CORE_VERSION})

message(STATUS "[INFO] Building ICOV core version ${ICOV_CORE_VERSION}")

include_directories(src)

add_subdirectory(src)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "[INFO] Building ICOV core tests")
    add_subdirectory(tests/unit_tests)
endif()

if(ICOV_EXTENSIONS_ENABLED)
    add_subdirectory(extensions)
endif()

#set(INSTALL_DEMO 1)

# message(${CMAKE_CURRENT_SOURCE_DIR})
# message(${CMAKE_SOURCE_DIR})
# message(${CMAKE_BINARY_DIR})

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