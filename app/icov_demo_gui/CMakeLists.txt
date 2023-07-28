message("BUILD ICOV DEMO GUI PROJECT")

# executable
set(EXE_NAME icov_demo_gui)

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(
  Qt5 REQUIRED
  COMPONENTS Core Widgets Xml Multimedia MultimediaWidgets
  REQUIRED)

add_executable(${EXE_NAME} main.cpp ${SRC_FILES} ${INC_FILES})

if(UNIX)
  target_link_libraries(${EXE_NAME} dl)
endif()

# set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
# ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXE_NAME})

target_link_libraries(
  ${EXE_NAME}
  Qt5::Core
  Qt5::Widgets
  Qt5::Xml
  Qt5::Multimedia
  Qt5::MultimediaWidgets
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
