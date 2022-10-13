message(STATUS "[INFO] Building ICOV core apps")

set(ICOV_BUILD_CLI ON CACHE BOOL "Build ICOV CLI app")
set(ICOV_BUILD_OPENCV_DEMO OFF CACHE BOOL "Build demo using OpenCV")

if(ICOV_BUILD_CLI)
    add_subdirectory(icov_cli)
endif()

if(ICOV_BUILD_OPENCV_DEMO)
    add_subdirectory(opencv_interactive_viewport)
endif()
