message("BUILD ICOV CLI PROJECT")

# executable
set(EXE_NAME icov_cli)

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h")

add_executable(${EXE_NAME} main.cpp ${SRC_FILES} ${INC_FILES})

add_dependencies(${EXE_NAME} glfw3_project glad_project)
target_include_directories(
  ${EXE_NAME}
  PRIVATE .
          ${OPENGL_INCLUDE_DIR}
          ${GLUT_INCLUDE_DIR}
          ${GLM_INCLUDE_DIRS}
          ${OpenCV_INCLUDE_DIRS}
          ${glad_INSTALL_DIR}/include
          ${glfw3_INSTALL_DIR}/include)
target_link_directories(${EXE_NAME} PRIVATE ${glad_INSTALL_DIR}/lib
                        ${glfw3_INSTALL_DIR}/lib64 ${glfw3_INSTALL_DIR}/lib)

if(UNIX)
  target_link_libraries(${EXE_NAME} dl)
endif()

target_link_libraries(${EXE_NAME} glfw3 glad ${OPENGL_LIBRARIES}
                      ${OPENGL_gl_LIBRARY} ${GLUT_LIBRARY})
set_source_files_properties("*.c" PROPERTIES LANGUAGE "CXX")

# set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
# ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXE_NAME})

target_link_libraries(
  ${EXE_NAME}
  ${Boost_LIBRARIES}
  ${OpenCV_LIBS}
  IcovCoreLib
  IcovLdpcCodecLayer
  IcovOmnidirectionalLayer
  IcovEncoderLib
  IcovExtractorLib
  IcovDecoderLib)
# set the folder where to place the projects
set_target_properties(${EXE_NAME} PROPERTIES FOLDER "public/app")
