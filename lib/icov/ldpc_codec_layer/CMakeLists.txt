########################################
#           ldpc_codec_layer          #
########################################

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")

add_library(IcovLdpcCodecLayer ${SRC_FILES} ${INC_FILES})
target_include_directories(IcovLdpcCodecLayer PRIVATE .)
target_link_libraries(IcovLdpcCodecLayer IcovCoreLib aff3ct::aff3ct-static-lib ${Boost_LIBRARIES} Threads::Threads)
set_target_properties(IcovLdpcCodecLayer PROPERTIES FOLDER "public/lib")

