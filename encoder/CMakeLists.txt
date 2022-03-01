cmake_minimum_required(VERSION 3.10)

find_package(OpenCV REQUIRED)

# library
set( LIB_NAME IcovEncoderLib )

include_directories(. ${OpenCV_INCLUDE_DIRS})

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp" )

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")

# library
add_library( ${LIB_NAME} SHARED ${SRC_FILES} ${INC_FILES})
add_library( s_${LIB_NAME} STATIC ${SRC_FILES} ${INC_FILES})

#install(TARGETS ${LIB_NAME} DESTINATION ./lib EXPORT icov-targets)
#install(EXPORT icov-targets DESTINATION lib/icov)

#target_link_libraries(${LIB_NAME})
target_link_libraries(s_${LIB_NAME} s_IcovCoreLib ${OpenCV_LIBS})
