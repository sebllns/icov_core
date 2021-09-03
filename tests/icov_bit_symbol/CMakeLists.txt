message("BUILD ICOV BIT TO SYMBOL TEST PROJECT")

# executable
set( EXE_NAME icov_bit_symbol_test )

# get source files
file(GLOB SRC_FILES "*.cpp")

# get include files
file(GLOB INC_FILES "*.h")

add_executable(${EXE_NAME} main.cpp ${SRC_FILES} ${INC_FILES})


include_directories(.)
#include_directories(${CMAKE_CURRENT_BINARY_DIR})


target_link_libraries(${EXE_NAME} s_IcovCoreLib aff3ct::aff3ct-static-lib)

