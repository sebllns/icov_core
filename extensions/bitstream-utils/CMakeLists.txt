cmake_minimum_required(VERSION 3.10)

message("BUILD BITSTREAM-UTILS EXTENSION")

find_package(Boost 1.45.0 REQUIRED) 


#add_compile_options(-Wall)


# library
get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" LIB_NAME ${LIB_NAME})

# get source files
file(GLOB_RECURSE SRC_FILES "*.c" "*.cpp" )

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" )


include_directories(. ${Boost_INCLUDE_DIRS})

# library
add_library( ${LIB_NAME} SHARED ${SRC_FILES} ${INC_FILES})
add_library( s_${LIB_NAME} STATIC ${SRC_FILES} ${INC_FILES})

target_link_libraries(${LIB_NAME} ${Boost_LIBRARIES})
target_link_libraries(s_${LIB_NAME} ${Boost_LIBRARIES})

#install(TARGETS ${LIB_NAME} DESTINATION ./lib EXPORT icov-targets)
#install(EXPORT icov-targets DESTINATION lib/icov)

