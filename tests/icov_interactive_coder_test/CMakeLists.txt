message("BUILD ICOV INTERACTIVE CODER TEST PROJECT")

# executable
set( EXE_NAME icov_interactive_coder_test )

# get source files
file(GLOB SRC_FILES "*.cpp")

# get include files
file(GLOB INC_FILES "*.h")

add_executable(${EXE_NAME} main.cpp ${SRC_FILES} ${INC_FILES})


include_directories(.)
#include_directories(${CMAKE_CURRENT_BINARY_DIR})


target_link_libraries(${EXE_NAME}  s_IcovRendererLib s_IcovNetworkLib s_IcovCoreLib aff3ct::aff3ct-static-lib)

