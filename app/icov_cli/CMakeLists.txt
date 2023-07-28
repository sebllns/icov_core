message("BUILD ICOV CLI PROJECT")

# executable
set(EXE_NAME icov_cli)

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h")

add_executable(${EXE_NAME} main.cpp ${SRC_FILES} ${INC_FILES})

target_include_directories(${EXE_NAME} PRIVATE . ${APP_INCLUDE_DIRS})

if(UNIX)
  target_link_libraries(${EXE_NAME} dl)
endif()

set_source_files_properties("*.c" PROPERTIES LANGUAGE "CXX")

# set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
# ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXE_NAME})

target_link_libraries(
  ${EXE_NAME}
  IcovCoreLib
  IcovLdpcCodecLayer
  IcovOmnidirectionalLayer
  IcovEncoderLib
  IcovExtractorLib
  IcovDecoderLib
  IcovGlViewer)
# set the folder where to place the projects
set_target_properties(${EXE_NAME} PROPERTIES FOLDER "public/app")
