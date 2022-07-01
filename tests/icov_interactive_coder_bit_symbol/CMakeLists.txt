message("BUILD ICOV INTERACTIVE CODER BIT SYMBOL PROJECT")

# executable
set( EXE_NAME icov_interactive_coder_bit_symbol )

# get source files
file(GLOB SRC_FILES "*.cpp")

# get include files
file(GLOB INC_FILES "*.h")

add_executable(${EXE_NAME} main.cpp ${SRC_FILES} ${INC_FILES})


include_directories(.)
#include_directories(${CMAKE_CURRENT_BINARY_DIR})


target_link_libraries(${EXE_NAME} s_IcovCoreLib)

# set the folder where to place the projects
set_target_properties( ${EXE_NAME} PROPERTIES FOLDER "public/tests" )
