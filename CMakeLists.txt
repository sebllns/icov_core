cmake_minimum_required(VERSION 3.10)

message(BUILD ICOV CORE PROJECT)

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
    add_subdirectory(tests)
endif()

#set(INSTALL_DEMO 1)

if(INSTALL_DEMO)
    make_directory(demo)

    if(NOT DEFINED DEMO_VERSION)
        file(DOWNLOAD url_version.txt demo/version.txt)
        file(STRINGS "version.txt" ICOV_DEMO_VERSION)
    endif()

    #write version
    #make_directory(ICOV_DEMO_VERSION)


    message(STATUS "[INFO] Downloading demo")
    file(DOWNLOAD https://downloads.sourceforge.net/project/icov/icov-demonstrator/icov-equirect-sample/v1/linux/demo-linux.zip demo-linux.zip)

    message(STATUS "[INFO] Extracting demo")
    #file(ARCHIVE_EXTRACT INPUT <archive> [...])
endif()