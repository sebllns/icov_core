cmake_minimum_required(VERSION 3.10)

message(STATUS "ICOV EXTENSIONS ENABLED")

set(ICOV_ALL_EXTENSIONS_ENABLED ON CACHE BOOL "Force usage of all extensions")

if(ICOV_ALL_EXTENSIONS_ENABLED)
    set(BITSTREAM_UTILS_ENABLED ON)
endif()


if(BITSTREAM_UTILS_ENABLED)
    add_subdirectory(bitstream-utils)
endif()
