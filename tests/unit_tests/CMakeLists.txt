find_package(Boost 1.70.0
    COMPONENTS
    unit_test_framework
    REQUIRED)


# aff3ct is imported in lib/icov/ldpc_codec_layer only, import it here too
find_package(AFF3CT CONFIG 2.3.5 REQUIRED)

message(STATUS "[INFO] Building ICOV core unit tests")
set(SRC icovunittest.cc ldpca_coder_tests.cc)
add_executable(icov_core_unit_tests ${SRC})
target_link_libraries(icov_core_unit_tests IcovLdpcCodecLayer IcovCoreLib Boost::unit_test_framework)
target_compile_definitions(icov_core_unit_tests PRIVATE
    ICOV_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/data/imt_164_v2/")
# set the folder where to place the projects
set_target_properties(icov_core_unit_tests PROPERTIES FOLDER "public/tests" )

add_test(NAME unit_tests COMMAND icov_core_unit_tests
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(unit_tests PROPERTIES LABELS unit)
