cmake_minimum_required(VERSION 3.10)

project(icov_core LANGUAGES CXX)

# library
set( LIB_NAME IcovCoreLib )


include_directories(src)

add_subdirectory(src/common)
#add_subdirectory(src/encoder)
add_subdirectory(src/decoder)
add_subdirectory(src/extractor)

# library
add_library( ${LIB_NAME} SHARED "src/icov_core.cpp")

target_link_libraries(${LIB_NAME} IcovDecoderLib IcovCommonLib IcovExtractorLib)
