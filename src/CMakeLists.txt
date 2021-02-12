cmake_minimum_required(VERSION 3.10)

project(icov_core LANGUAGES CXX)

# library
set( LIB_NAME IcovCoreLib )


# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp" )

# get include files
file(GLOB_RECURSE INC_FILES "*.h" )


include_directories(.)

# library
add_library( ${LIB_NAME} SHARED ${SRC_FILES} ${INC_FILES})
