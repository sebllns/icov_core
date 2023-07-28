message(STATUS "[INFO] Building ICOV core apps")

set(ICOV_BUILD_CLI
    ON
    CACHE BOOL "Build ICOV CLI app")
set(ICOV_BUILD_GUI_DEMO
    OFF
    CACHE BOOL "Build demo using Qt")

if(ICOV_WITH_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY
      "clang-tidy;-checks=-*,clang-analyzer-*,boost-*,performance-*,cppcoreguidelines-owning-memory,portability-*"
  )
endif()

if(UNIX)
  add_compile_options("-Wno-write-strings")
  # lib dl
endif()

if(ICOV_BUILD_CLI)
  add_subdirectory(icov_cli)
endif()

if(ICOV_BUILD_GUI_DEMO)
  add_subdirectory(icov_demo_gui)
endif()
