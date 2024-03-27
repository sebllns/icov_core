########################################
#           ldpc_codec_layer          #
########################################

# ---------------------------------------------------------------------
# Link with AFF3CT
message(STATUS "[INFO] Checking AFF3CT support...")
find_package(
  AFF3CT
  CONFIG
  2.3.5
  REQUIRED
 # NO_DEFAULT_PATH
)
message(STATUS "[INFO] FOUND AFF3CT: ${AFF3CT_CONFIG}")
message("\n")
message(
  "-------------------------------------------------------------------------")

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")

add_library(IcovLdpcCodecLayer ${SRC_FILES} ${INC_FILES})
target_include_directories(IcovLdpcCodecLayer PRIVATE .)
target_link_libraries(IcovLdpcCodecLayer IcovCoreLib aff3ct::aff3ct-static-lib ${Boost_LIBRARIES} Threads::Threads)
set_target_properties(IcovLdpcCodecLayer PROPERTIES FOLDER "public/lib")

