########################################
#                CORE                  #
########################################
# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")

#static library
add_library(IcovCoreLib ${SRC_FILES} ${INC_FILES})
target_include_directories(IcovCoreLib PUBLIC . common common/dev_tools ldpc_codec_layer omnidirectional_layer video_layer)
target_link_libraries(IcovCoreLib PUBLIC ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
set_target_properties(IcovCoreLib PROPERTIES FOLDER "public/lib")



#install(TARGETS ${LIB_NAME} DESTINATION ./lib EXPORT icov-targets)
#install(EXPORT icov-targets DESTINATION lib/icov)
