########################################
#        omnidirectional_layer         #
########################################
# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")

add_library(IcovOmnidirectionalLayer ${SRC_FILES} ${INC_FILES})
target_include_directories(IcovOmnidirectionalLayer PRIVATE .)
target_link_libraries(IcovOmnidirectionalLayer IcovCoreLib ${Boost_LIBRARIES} Threads::Threads)
set_target_properties(IcovOmnidirectionalLayer PROPERTIES FOLDER "public/lib")
