cmake_minimum_required(VERSION 3.10)

find_package(OpenCV REQUIRED)

# library
set(LIB_NAME IcovDecoderLib)

include_directories(${OpenCV_INCLUDE_DIRS})

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")

# library
add_library(${LIB_NAME} ${SRC_FILES} ${INC_FILES})
target_link_libraries(${LIB_NAME} IcovCoreLib IcovLdpcCodecLayer
                      IcovOmnidirectionalLayer IcovVideoLayer ${OpenCV_LIBS})
set_target_properties(${LIB_NAME} PROPERTIES FOLDER "public/lib")
