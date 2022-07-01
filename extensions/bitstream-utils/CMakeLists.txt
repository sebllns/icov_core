message(STATUS "[INFO] bitstream-utils extension ENABLED")

# library
get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" LIB_NAME ${LIB_NAME})

# get source files
file(GLOB_RECURSE SRC_FILES "*.c" "*.cpp" )

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" )

include_directories(.)

# library
add_library( ${LIB_NAME} SHARED ${SRC_FILES} ${INC_FILES})
add_library( s_${LIB_NAME} STATIC ${SRC_FILES} ${INC_FILES})

target_include_directories(${LIB_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(s_${LIB_NAME} PUBLIC ${Boost_INCLUDE_DIRS})

target_link_libraries(${LIB_NAME} ${Boost_LIBRARIES})
target_link_libraries(s_${LIB_NAME} ${Boost_LIBRARIES})

# set the folder where to place the projects
set_target_properties( ${LIB_NAME} PROPERTIES FOLDER "public/lib/shared" )
set_target_properties( s_${LIB_NAME} PROPERTIES FOLDER "public/lib/static" )