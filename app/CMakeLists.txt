message(STATUS "[INFO] Building ICOV core apps")

option(ICOV_BUILD_CLI "Build ICOV CLI app" ON)
option(ICOV_BUILD_GL_VIEWER "Build GL viewer demo" ON)
option(ICOV_BUILD_GUI_DEMO "Build demo using Qt" OFF)

if(ICOV_WITH_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY
      "clang-tidy;-checks=-*,clang-analyzer-*,boost-*,performance-*,cppcoreguidelines-owning-memory,portability-*"
  )
endif()

if(UNIX)
  add_compile_options("-Wno-write-strings")
  # lib dl
endif()

link_libraries(
  ${Boost_LIBRARIES}
  ${OpenCV_LIBS}
  ${OpenCV_LIBRARIES}
  IcovCoreLib
  IcovLdpcCodecLayer
  IcovOmnidirectionalLayer
  IcovEncoderLib
  IcovExtractorLib
  IcovDecoderLib
  IcovAppCommonLib)

if(ICOV_BUILD_CLI)
  add_subdirectory(icov_cli)
endif()

if(ICOV_BUILD_GL_VIEWER)
  add_subdirectory(icov_demo_gl)
endif()

if(ICOV_BUILD_GUI_DEMO)
  add_subdirectory(icov_demo_gui)
endif()
