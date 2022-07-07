message(STATUS "[INFO] Building ICOV core apps")

set(BUILD_ICOV_CLI ON CACHE BOOL "Build ICOV CLI app")
set(BUILD_OPENCV_DEMO OFF CACHE BOOL "Build demo using OpenCV")

if(BUILD_ICOV_CLI)
    add_subdirectory(icov_cli)
endif()

if(BUILD_OPENCV_DEMO)
    add_subdirectory(opencv_interactive_viewport)
endif()