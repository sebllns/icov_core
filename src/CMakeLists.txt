cmake_minimum_required(VERSION 3.10)




#SET(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/client_application/UnityProject/Assets/Plugins/)

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

#install(TARGETS ${LIB_NAME} DESTINATION ./lib EXPORT icov-targets)
#install(EXPORT icov-targets DESTINATION lib/icov)

