########################################
#                CORE                  #
########################################
# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")

add_library(IcovCoreLib ${SRC_FILES} ${INC_FILES})
target_include_directories(IcovCoreLib PUBLIC . common)# common/dev_tools ldpc_codec_layer omnidirectional_layer video_layer)
target_link_libraries(IcovCoreLib PUBLIC ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
if(ICOV_USE_OMP)
    add_compile_definitions(ICOV_USE_OMP)
    target_link_libraries(IcovCoreLib PUBLIC OpenMP::OpenMP_CXX)
endif()
set_target_properties(IcovCoreLib PROPERTIES FOLDER "public/lib")
