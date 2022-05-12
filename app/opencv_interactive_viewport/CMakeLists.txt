message("BUILD ICOV VIEWER PROJECT")

#set(VAL_FIND_DEBUG_MODE ${CMAKE_FIND_DEBUG_MODE})
#set(CMAKE_FIND_DEBUG_MODE TRUE)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(OpenCV REQUIRED)
find_package(glm REQUIRED)
#find_package(glfw3 3.3 REQUIRED)


include(ExternalProject)

message("GIT_EXECUTABLE='${GIT_EXECUTABLE}'")

set(glfw3_PREFIX "${CMAKE_BINARY_DIR}/third-party/glfw3")
set(glfw3_INSTALL_DIR "${glfw3_PREFIX}")
set(glfw3_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${glfw3_INSTALL_DIR} -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF)

message("glfw3_PREFIX='${glfw3_PREFIX}'")
message("glfw3_INSTALL_DIR='${glfw3_INSTALL_DIR}'")
message("glfw3_CMAKE_ARGS='${glfw3_CMAKE_ARGS}'")
ExternalProject_Add(glfw3_project
    PREFIX ${glfw3_PREFIX}
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    INSTALL_DIR ${glfw3_INSTALL_DIR}
    CMAKE_ARGS ${glfw3_CMAKE_ARGS})

#find_package(glfw3 3.3 REQUIRED)

message("glfw3='${glfw3_INSTALL_DIR}'")


set(glad_PREFIX "${CMAKE_BINARY_DIR}/third-party/glad")
set(glad_INSTALL_DIR "${glad_PREFIX}")
set(glad_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${glad_INSTALL_DIR} -DGLAD_INSTALL=ON)

message("glad_PREFIX='${glad_PREFIX}'")
message("glad_INSTALL_DIR='${glad_INSTALL_DIR}'")
message("glad_CMAKE_ARGS='${glad_CMAKE_ARGS}'")
ExternalProject_Add(glad_project
    PREFIX ${glad_PREFIX}
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    INSTALL_DIR ${glad_INSTALL_DIR}
    CMAKE_ARGS ${glad_CMAKE_ARGS})

#find_package(GLFW REQUIRED)
#set(CMAKE_FIND_DEBUG_MODE FALSE)
#set(CMAKE_FIND_DEBUG_MODE ${VAL_FIND_DEBUG_MODE})

message(STATUS "OPENGL_INCLUDE_DIR := ${OPENGL_INCLUDE_DIR}")
#message(STATUS "OPENGL_LIBRARIES:= ${OPENGL_LIBRARIES}")
message(STATUS "GLUT_INCLUDE_DIR := ${GLUT_INCLUDE_DIR}")
message(STATUS "OpenCV = ${OpenCV_INCLUDE_DIRS}")
message(STATUS "glm = ${GLM_INCLUDE_DIRS}")
message(STATUS "glad = ${glad_LIBRARY}")

if(UNIX)
add_compile_options("-Wno-write-strings")
#lib dl
endif()

# executable
set( EXE_NAME icov_opencv_interactive_viewport )

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXE_NAME})
#set(EXECUTABLE_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH}/icov_viewport_construction)

#set(GLFW_BUILD_EXAMPLES OFF)
#set(GLFW_BUILD_TESTS OFF)
#set(GLFW_BUILD_DOCS OFF)

#add_subdirectory(glm-0.9.9.0)
#add_subdirectory(glad_gl460)
#add_subdirectory(glfw-3.3.2)

# get source files
file(GLOB SRC_FILES "*.cpp")

# get include files
file(GLOB INC_FILES "*.h")


add_executable(${EXE_NAME}  main.cpp ${SRC_FILES} ${INC_FILES})
add_dependencies(${EXE_NAME}  glfw3_project glad_project)

include_directories(. ${OPENGL_INCLUDE_DIR} ${GLUT_INCLUDE_DIR} ${GLM_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${glad_INSTALL_DIR}/include ${glfw3_INSTALL_DIR}/include)

target_link_directories(${EXE_NAME} PRIVATE ${glad_INSTALL_DIR}/lib ${glfw3_INSTALL_DIR}/lib64 ${glfw3_INSTALL_DIR}/lib)

if(UNIX)
    target_link_libraries(${EXE_NAME}  dl)
endif()

target_link_libraries(${EXE_NAME} s_IcovRendererLib s_IcovNetworkLib s_IcovCoreLib s_IcovEncoderLib glfw3 glad ${OPENGL_LIBRARIES} ${OPENGL_gl_LIBRARY} ${GLUT_LIBRARY} ${OpenCV_LIBS})
set_source_files_properties( "*.c" PROPERTIES LANGUAGE "CXX" )

set_target_properties(${EXE_NAME}
PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXE_NAME})
