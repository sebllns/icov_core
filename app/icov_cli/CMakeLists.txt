message("BUILD ICOV CLI PROJECT")

# executable
set(EXE_NAME icov_cli)

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h")

add_executable(${EXE_NAME} main.cpp ${SRC_FILES} ${INC_FILES})

add_dependencies(${EXE_NAME} glfw3_project glad_project)

set(APP_INCLUDE_DIRS ${OPENGL_INCLUDE_DIR}
${GLUT_INCLUDE_DIR}
${GLM_INCLUDE_DIRS}
${OpenCV_INCLUDE_DIRS}
${glad_INSTALL_DIR}/include
${glfw3_INSTALL_DIR}/include)

set(APP_LINK_DIRS ${glad_INSTALL_DIR}/lib
${glfw3_INSTALL_DIR}/lib64 ${glfw3_INSTALL_DIR}/lib)

set(APP_GL_LINK_LIBS glfw3 glad glm::glm ${OPENGL_LIBRARIES}
${OPENGL_gl_LIBRARY} ${GLUT_LIBRARY})

set(APP_LINK_LIBS
${Boost_LIBRARIES}
${OpenCV_LIBS}
IcovCoreLib
IcovLdpcCodecLayer
IcovOmnidirectionalLayer
IcovEncoderLib
IcovExtractorLib
IcovDecoderLib)

message(STATUS "[INFO] App include: ${APP_INCLUDE_DIRS}")
message(STATUS "---------------------------------------------------------------------------\n\n")
message(STATUS "[INFO] App link folders: ${APP_LINK_DIRS}")
message(STATUS "---------------------------------------------------------------------------\n\n")
message(STATUS "[INFO] App GL link librairies: ${APP_GL_LINK_LIBS}")
message(STATUS "---------------------------------------------------------------------------\n\n")
message(STATUS "[INFO] App link librairies: ${APP_LINK_LIBS}")
message(STATUS "---------------------------------------------------------------------------\n\n")

target_include_directories(
  ${EXE_NAME}
  PRIVATE .
          ${APP_INCLUDE_DIRS})

target_link_directories(${EXE_NAME} PRIVATE ${APP_LINK_DIRS})

if(UNIX)
  target_link_libraries(${EXE_NAME} dl)
endif()

#TODO: encasuplate with if (make openGL optional feature)
target_link_libraries(${EXE_NAME} ${APP_GL_LINK_LIBS})

set_source_files_properties("*.c" PROPERTIES LANGUAGE "CXX")

# set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
# ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXE_NAME})

target_link_libraries(
  ${EXE_NAME}
  ${APP_LINK_LIBS})
# set the folder where to place the projects
set_target_properties(${EXE_NAME} PROPERTIES FOLDER "public/app")
