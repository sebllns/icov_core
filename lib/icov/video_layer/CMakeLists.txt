# ##############################################################################
# video_layer         #
# ##############################################################################

find_package(OpenCV REQUIRED)

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")

add_library(IcovVideoLayer ${SRC_FILES} ${INC_FILES})
target_include_directories(IcovVideoLayer PRIVATE . SYSTEM
                                                  ${OpenCV_INCLUDE_DIRS})
target_link_libraries(
  IcovVideoLayer
  IcovCoreLib
  ${Boost_LIBRARIES}
  Threads::Threads
  CommonLib
  Utilities
  ${OpenCV_LIBS}
  ${OpenCV_LIBRARIES})
set_target_properties(IcovVideoLayer PROPERTIES FOLDER "public/lib")
