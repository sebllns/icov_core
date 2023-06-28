cmake_minimum_required(VERSION 3.10)

# add_compile_options(-Wall)

if(ICOV_WITH_CPPCHECK)
  find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
  if(CMAKE_CXX_CPPCHECK STREQUAL "CMAKE_CXX_CPPCHECK-NOTFOUND")
    message(
      FATAL_ERROR
        "[FATAL] ENABLE_CPPCHECK is set to TRUE but cppcheck not found. Please regenerate with ENABLE_CPPCHECK=FALSE"
    )
  else()
    message(STATUS "[INFO] Use cppcheck")
    list(
      APPEND CMAKE_CXX_CPPCHECK "-j 4"
      "--suppressions-list=${CMAKE_SOURCE_DIR}/.cmake/CppCheckSuppressions.txt")
  endif()
endif()

if(ICOV_WITH_CLANG_TIDY)
  message(STATUS "[INFO] Use clang-tidy")
  set(CMAKE_CXX_CLANG_TIDY
      "clang-tidy;-checks=-*,clang-analyzer-*,boost-*,performance-*,cppcoreguidelines-owning-memory,portability-*"
  )
  # ,readability-*
endif()

include_directories(. icov icov/core encoder extensions)
add_subdirectory(core)
add_subdirectory(ldpc_codec_layer)
add_subdirectory(omnidirectional_layer)
add_subdirectory(video_layer)
