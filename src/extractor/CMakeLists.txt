# library
set( LIB_NAME IcovExtractorLib )

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp" )

# get include files
file(GLOB_RECURSE INC_FILES "*.h" )

# library
add_library( ${LIB_NAME} STATIC ${SRC_FILES} ${INC_FILES})

target_include_directories( ${LIB_NAME} PUBLIC .)

target_link_libraries(${LIB_NAME} IcovCommonLib)

# set the folder where to place the projects
set_target_properties( ${LIB_NAME} PROPERTIES FOLDER lib )
