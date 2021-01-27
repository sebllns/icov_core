cmake_minimum_required(VERSION 3.10)

project(icov_core LANGUAGES CXX)

# library
set( LIB_NAME IcovCoreLib )


add_subdirectory(common)
#add_subdirectory(encoder)
add_subdirectory(decoder)
add_subdirectory(extractor)

include_directories(.)

# library
add_library( ${LIB_NAME} SHARED "icov_core.cpp")

target_link_libraries(${LIB_NAME} IcovDecoderLib IcovCommonLib IcovExtractorLib)
