project(icov_core LANGUAGES CXX)

# library
set( LIB_NAME IcovCoreLib )


add_subdirectory(common)
add_subdirectory(decoder)
add_subdirectory(extractor)

include_directories(.)

# library
add_library( ${LIB_NAME} INTERFACE)

target_link_libraries(${LIB_NAME} INTERFACE IcovDecoderLib IcovCommonLib)
