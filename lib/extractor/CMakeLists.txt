cmake_minimum_required(VERSION 3.10)

# library
set( LIB_NAME IcovExtractorLib )

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp" )

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")

# library
add_library(${LIB_NAME} ${SRC_FILES} ${INC_FILES})
target_link_libraries(${LIB_NAME} IcovCoreLib IcovOmnidirectionalLayer IcovLdpcCodecLayer)
set_target_properties(${LIB_NAME} PROPERTIES FOLDER "public/lib")
