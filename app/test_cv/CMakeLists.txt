message("BUILD ICOV TEST CV PROJECT")

find_package(OpenCV REQUIRED)

# executable
set( EXE_NAME test_cv )

# get source files
file(GLOB SRC_FILES "*.cpp")

# get include files
file(GLOB INC_FILES "*.h")

if(ICOV_WITH_CLANG_TIDY)
set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=-*,clang-analyzer-*,boost-*,performance-*,cppcoreguidelines-owning-memory,portability-*")
endif()

add_executable(${EXE_NAME} main.cpp ${SRC_FILES} ${INC_FILES})


include_directories(. ${OpenCV_INCLUDE_DIRS})

target_link_libraries(${EXE_NAME} ${OpenCV_LIBS})
# set the folder where to place the projects
set_target_properties( ${EXE_NAME} PROPERTIES FOLDER "public/app" )
