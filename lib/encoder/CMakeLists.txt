cmake_minimum_required(VERSION 3.10)

find_package(OpenCV REQUIRED)

# library
set( LIB_NAME IcovEncoderLib )

include_directories(. ${OpenCV_INCLUDE_DIRS})

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp" )

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")

# library
#add_library( ${LIB_NAME} SHARED ${SRC_FILES} ${INC_FILES})
#target_link_libraries(${LIB_NAME} IcovCoreLib IcovLdpcCodecLayer IcovOmnidirectionalLayer IcovVideoLayer ${OpenCV_LIBS})
#set_target_properties( ${LIB_NAME} PROPERTIES FOLDER "public/lib/shared" )

add_library( s_${LIB_NAME} STATIC ${SRC_FILES} ${INC_FILES})
target_link_libraries(s_${LIB_NAME} s_IcovCoreLib s_IcovLdpcCodecLayer s_IcovOmnidirectionalLayer s_IcovVideoLayer ${OpenCV_LIBS})
set_target_properties( s_${LIB_NAME} PROPERTIES FOLDER "public/lib/static" )


#install(TARGETS ${LIB_NAME} DESTINATION ./lib EXPORT icov-targets)
#install(EXPORT icov-targets DESTINATION lib/icov)
