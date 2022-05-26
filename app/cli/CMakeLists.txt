message("BUILD ICOV CLI PROJECT")

find_package(Boost 1.70.0 COMPONENTS
    program_options
    filesystem
    REQUIRED)

# executable
set( EXE_NAME icov_cli )

# get source files
file(GLOB SRC_FILES "*.cpp")

# get include files
file(GLOB INC_FILES "*.h")

set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=-*,clang-analyzer-*,boost-*,performance-*,cppcoreguidelines-owning-memory,portability-*")

add_executable(${EXE_NAME} main.cpp ${SRC_FILES} ${INC_FILES})


include_directories(.)

target_link_libraries(${EXE_NAME} ${Boost_LIBRARIES} s_IcovCoreLib s_IcovEncoderLib)
