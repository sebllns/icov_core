cmake_minimum_required(VERSION 3.10)

# library
set(LIB_NAME IcovEncoderLib)

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")

# library
add_library(${LIB_NAME} ${SRC_FILES} ${INC_FILES})
target_include_directories(${LIB_NAME} PUBLIC ${ICOV_OPENCV_INCLUDE})
target_link_libraries(
  ${LIB_NAME} PUBLIC IcovCoreLib IcovLdpcCodecLayer IcovOmnidirectionalLayer
                     IcovVideoLayer ${ICOV_OPENCV_LIBS})
set_target_properties(${LIB_NAME} PROPERTIES FOLDER "public/lib")
