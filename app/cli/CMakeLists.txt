message("BUILD ICOV IMT ALIST GENERATOR PROJECT")

# executable
set( EXE_NAME icov_cli )

# get source files
file(GLOB SRC_FILES "*.cpp")

# get include files
file(GLOB INC_FILES "*.h")

set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=-*,clang-analyzer-*,boost-*,performance-*,cppcoreguidelines-owning-memory,portability-*")

add_executable(${EXE_NAME} main.cpp ${SRC_FILES} ${INC_FILES})


include_directories(.)
#include_directories(${CMAKE_CURRENT_BINARY_DIR})


target_link_libraries(${EXE_NAME}  s_IcovRendererLib s_IcovNetworkLib s_IcovCoreLib)
