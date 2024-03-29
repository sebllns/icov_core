# Specify the minimum version of CMake required to build this project
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

# ##############################################################################
# INIT PROJECT              #
# ##############################################################################

# Specify the project name and the languages it is written in
project(icov_core_superbuild LANGUAGES CXX C)

option(ICOV_CONFIGURE_WITH_CONAN "Configure ICOV project with conan package manager" OFF)
option(ICOV_ONLINE_MODE "Disable this option is you work offline" ON)
option(ICOV_BUILD_CORE "Build the icov core project" ON)
option(BUILD_SHARED_LIBS "Build shared libs" OFF)

# Display a message indicating that the project is being built
message(STATUS "START BUILD ICOV CORE SUPERBUILD")

# Include functions
message(VERBOSE "[TRACE] Load cmake includes")
include(${CMAKE_CURRENT_SOURCE_DIR}/.cmake/AddGitSubmodule.cmake)

if(NOT ICOV_ONLINE_MODE)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
               PROPERTY EP_UPDATE_DISCONNECTED 1)
endif()
#set_property (DIRECTORY PROPERTY EP_BASE Dependencies)
include(ExternalProject)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH};${CMAKE_BINARY_DIR}/third-party)

set (ICOV_DEPENDENCIES)
set (ICOV_EXTRA_CMAKE_ARGS)


if(ICOV_CONFIGURE_WITH_CONAN)
  message(STATUS "CONFIGURE ICOV CORE PROJECT WITH CONAN")
  find_program(CONAN conan)
  if(NOT CONAN)
      message(FATAL_ERROR "Conan not found. Please check https://conan.io/downloads")
  endif()

  execute_process(COMMAND ${CONAN} profile detect ERROR_QUIET)
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/conan/icov_profile" DESTINATION "${CMAKE_BINARY_DIR}")
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/conan/conanfile.txt" DESTINATION "${CMAKE_BINARY_DIR}")

  #message(FATAL_ERROR "FIXME: conan is disabled here")

  execute_process(COMMAND ${CONAN} install ${CMAKE_BINARY_DIR} --output-folder=${CMAKE_BINARY_DIR}/conan --build=missing -c tools.system.package_manager:mode=install --profile default --profile ${CMAKE_BINARY_DIR}/icov_profile -vvv COMMAND_ERROR_IS_FATAL ANY)

#on linux --settings compiler.libcxx="libstdc++11"
  # set(CMAKE_TOOLCHAIN_FILE ${CMAKE_BINARY_DIR}/conan/build/Release/generators/conan_toolchain.cmake)
   # execute_process(COMMAND ${CMAKE_COMMAND} .. -DCMAKE_TOOLCHAIN_FILE=${CMAKE_BINARY_DIR}/conan/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release --log-level=TRACE -DICOV_USE_OMP=ON -DICOV_CORE_DEBUG_TRACE_ENABLED=OFF -DICOV_CORE_STACKTRACE_ENABLED=ON -DICOV_CHECK_BITPLANE_ENCODING_ENABLED=OFF -DICOV_CONSOLE_COLOR_ENABLED=ON -DICOV_LOG_BOOST=OFF -DICOV_ENCODER_EXPORT_TEXT_ENABLED=ON  WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND_ERROR_IS_FATAL ANY)


   list (APPEND ICOV_EXTRA_CMAKE_ARGS
     -DCMAKE_TOOLCHAIN_FILE=${CMAKE_BINARY_DIR}/conan/build/generators/conan_toolchain.cmake)

  else()

    # ##############################################################################
    # INFOS COMPILER            #
    # ##############################################################################
    message(
      "-------------------------------------------------------------------------")
    if(CMAKE_CXX_COMPILER_LOADED)
      message(STATUS "C++ Compiler path: ${CMAKE_CXX_COMPILER}")
      message(STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
      message(STATUS "C++ Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
      if(CMAKE_COMPILER_IS_GNUCXX)
        message(STATUS "C++ Compiler is part of GCC")
        execute_process(COMMAND "${CMAKE_CXX_COMPILER} -v" COMMAND_ECHO STDOUT)  
        if ((CMAKE_BUILD_TYPE MATCHES Release))
          set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
        endif()
      endif()
    endif()
    message("\n")

    if(CMAKE_C_COMPILER_LOADED)
      message(STATUS "C Compiler path: ${CMAKE_C_COMPILER}")
      message(STATUS "C Compiler ID: ${CMAKE_C_COMPILER_ID}")
      message(STATUS "C Compiler version: ${CMAKE_C_COMPILER_VERSION}")
      if(CMAKE_COMPILER_IS_GNUC)
        message(STATUS "C Compiler is part of GCC")
      endif()
    else()
      message(FATAL_ERROR "[ERROR] No C compiler defined")
    endif()

    message("\n")
    message(STATUS "Cmake infos")
    execute_process(COMMAND "${CMAKE_COMMAND}" --version COMMAND_ECHO STDOUT)

    # ##############################################################################
    # CHECK DEPENDENCIES            #
    # ##############################################################################
    message(
      "-------------------------------------------------------------------------")
    message(STATUS "Checking dependencies...\n"
    )# cf https://cmake.org/cmake/help/latest/command/message.html
    message(
      "-------------------------------------------------------------------------")

    # ---------------------------------------------------------------------
    # Link with Boost

    #gcc, msvc, intel-linux, intel-win, acc, borland, como-linux, cw, dmc, hp_cxx, sun
    set( Boost_Bootstrap_Command )
    if( UNIX )
      set( Boost_Bootstrap_Command ./bootstrap.sh )
      set( Boost_b2_Command ./b2 )
    else()
      if( WIN32 )
          set(Boost_Toolset gcc)
          if (MSVC)
              set(Boost_Toolset msvc)
              set( Boost_b2_Command b2 toolset=${Boost_Toolset} --model=32) #
              else()
              set( Boost_b2_Command b2 toolset=${Boost_Toolset} --without-stacktrace )
          endif()
          set( Boost_Bootstrap_Command bootstrap.bat ${Boost_Toolset} )
      endif()
    endif()

    # Use static linking to avoid issues with system-wide installations of Boost.
    list (APPEND DEPENDENCIES boost)
    ExternalProject_Add (boost
      URL https://sourceforge.net/projects/boost/files/boost/1.81.0/boost_1_81_0.tar.bz2/download
      URL_MD5 3276c0637d1be8687740c550237ef999
      CONFIGURE_COMMAND ${Boost_Bootstrap_Command}
      BUILD_COMMAND ${Boost_b2_Command}
      link=static #shared,
      #runtime-link=static
      variant=release
      --build-type=minimal
      --without-python
      --without-fiber
      --without-url
      --without-mpi
      --without-graph_parallel
      --disable-icu
      --threading=single
      install --prefix=${CMAKE_BINARY_DIR}/third-party/boost
      BUILD_IN_SOURCE 1
      INSTALL_COMMAND ""
      )

  list (APPEND ICOV_DEPENDENCIES boost)
  list (APPEND ICOV_EXTRA_CMAKE_ARGS
    -DBoost_ROOT=${CMAKE_CURRENT_BINARY_DIR}/third-party/boost
    -DBoost_NO_SYSTEM_PATHS=ON)

    # ---------------------------------------------------------------------
    # Link with OpenCV
    message(STATUS "[INFO] Checking OpenCV support...")


    ExternalProject_Add(opencv
      URL https://github.com/opencv/opencv/archive/4.6.0.zip
      SOURCE_DIR opencv
      BINARY_DIR opencv-build
      INSTALL_DIR third-party/opencv
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      CMAKE_ARGS
        -DBUILD_DOCS:BOOL=OFF
        -DBUILD_EXAMPLES:BOOL=OFF
        -DBUILD_NEW_PYTHON_SUPPORT:BOOL=OFF
        -DBUILD_PACKAGE:BOOL=OFF
        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        -DBUILD_TESTS:BOOL=OFF
        -DCMAKE_BUILD_TYPE:STRING=Release
        -DWITH_FFMPEG:BOOL=ON
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/third-party/opencv
    )

list (APPEND ICOV_DEPENDENCIES opencv)
list (APPEND ICOV_EXTRA_CMAKE_ARGS
  -DOpenCV_DIR=${CMAKE_BINARY_DIR}/third-party/opencv)
  list (APPEND CMAKE_PREFIX_PATH
  ${CMAKE_BINARY_DIR}/third-party/opencv)

ExternalProject_Add(freeglut
  URL https://sourceforge.net/projects/freeglut/files/freeglut/3.4.0/freeglut-3.4.0.tar.gz/download
  URL_MD5 f1621464e6525d0368976870cab8f418
  SOURCE_DIR freeglut
  BINARY_DIR freeglut-build
  INSTALL_DIR third-party/freeglut
  UPDATE_COMMAND ""
  PATCH_COMMAND ""
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/third-party/freeglut
)

list (APPEND ICOV_DEPENDENCIES freeglut)
list (APPEND ICOV_EXTRA_CMAKE_ARGS
  -DFREEGLUT_DIR=${CMAKE_BINARY_DIR}/third-party/freeglut)
list (APPEND CMAKE_PREFIX_PATH
  ${CMAKE_BINARY_DIR}/third-party/freeglut/lib)


ExternalProject_Add(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git

  #URL https://github.com/g-truc/glm/releases/download/1.0.1/glm-1.0.1-light.zip
  SOURCE_DIR glm
  BINARY_DIR glm-build
  INSTALL_DIR third-party/glm
  UPDATE_COMMAND ""
  PATCH_COMMAND ""
  #INSTALL_COMMAND ""
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/third-party/glm
)

list (APPEND ICOV_DEPENDENCIES glm)
list (APPEND ICOV_EXTRA_CMAKE_ARGS
  -DGLM_DIR=${CMAKE_BINARY_DIR}/third-party/glm)
list (APPEND CMAKE_PREFIX_PATH
  ${CMAKE_BINARY_DIR}/glm)

endif(ICOV_CONFIGURE_WITH_CONAN)


# ---------------------------------------------------------------------
# Link with AFF3CT
message(STATUS "[INFO] Enable AFF3CT superbuild...")
set(AFF3CT_BUILD_DIR
    third-party/aff3ct
    CACHE
      STRING
      "Path to aff3ct build directory default is ${CMAKE_BINARY_DIR}/third-party/aff3ct"
)
set(AFF3CT_BUILD_DIR_FULL ${CMAKE_BINARY_DIR}/${AFF3CT_BUILD_DIR})
if(NOT DEFINED AFF3CT_BUILD_TYPE)
  set(AFF3CT_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/.cmake/install_aff3ct.cmake)
list (APPEND ICOV_EXTRA_CMAKE_ARGS
  -DAFF3CT_DIR=${CMAKE_BINARY_DIR}/third-party/aff3ct/lib/cmake/aff3ct-3.0.1-21-g72af2d3)
list (APPEND ICOV_DEPENDENCIES aff3ct)

message("\n")
message(
  "-------------------------------------------------------------------------")


if(ICOV_BUILD_CORE)

  set(ICOV_USE_OMP OFF)
  find_package(OpenMP)
  if(OpenMP_FOUND)
    message(STATUS "[INFO] OpenMP enabled...")
    set(ICOV_USE_OMP ON)
  endif()

  list (APPEND ICOV_EXTRA_CMAKE_ARGS
    -DICOV_BUILD_APPS=ON
    -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
    -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
    -DICOV_CORE_STACKTRACE_ENABLED=OFF
    -DGLM_LINK=glm::glm
    -DICOV_LOG_BOOST=OFF
    -DICOV_CONSOLE_COLOR_ENABLED=ON
    -DICOV_LOG_LEVEL=0
    -DICOV_USE_OMP=${ICOV_USE_OMP}
    -DICOV_ONLINE_MODE=${ICOV_ONLINE_MODE}
    #-DOPENGL_LIBRARY_DIR=${CMAKE_BINARY_DIR}/third-party/freeglut/lib
  )
    message(STATUS "CONFIGURE ICOV with CMAKE ARGS ${ICOV_EXTRA_CMAKE_ARGS}")
    ExternalProject_Add (icov
      DEPENDS ${ICOV_DEPENDENCIES}
      SOURCE_DIR ${PROJECT_SOURCE_DIR}/..
      CMAKE_ARGS ${ICOV_EXTRA_CMAKE_ARGS}
      INSTALL_COMMAND ""
      BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/icov)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/icov/configure.bat "\"${CMAKE_COMMAND}\" \"${PROJECT_SOURCE_DIR}/..\" ${ICOV_EXTRA_CMAKE_ARGS}")
endif()
