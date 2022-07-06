########################################
#        omnidirectional_layer         #
########################################
# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")


# shared library
# add_library(IcovCoreLib SHARED ${CORE_SRC_FILES} ${CORE_INC_FILES})
# target_include_directories(IcovCoreLib} PUBLIC . common common/dev_tools omnidirectional_layer omnidirectional_layer video_layer)
# target_link_libraries(IcovCoreLib aff3ct::aff3ct-static-lib ${Boost_LIBRARIES} Threads::Threads ${CMAKE_DL_LIBS} CommonLib)
# set_target_properties(IcovCoreLib PROPERTIES FOLDER "public/lib/shared")

#static library
add_library(s_IcovOmnidirectionalLayer STATIC ${SRC_FILES} ${INC_FILES})
target_include_directories(s_IcovOmnidirectionalLayer PRIVATE .)
target_link_libraries(s_IcovOmnidirectionalLayer s_IcovCoreLib ${Boost_LIBRARIES} Threads::Threads)
set_target_properties(s_IcovOmnidirectionalLayer PROPERTIES FOLDER "public/lib/static")



#install(TARGETS ${LIB_NAME} DESTINATION ./lib EXPORT icov-targets)
#install(EXPORT icov-targets DESTINATION lib/icov)
