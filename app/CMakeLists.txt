message(STATUS "[INFO] Building ICOV core apps")

add_subdirectory(icov_cli)
if(ICOV_PLATFORM MATCHES LINUX OR ICOV_PLATFORM MATCHES WIN)
    add_subdirectory(opencv_interactive_viewport)
endif()