find_package(Boost 1.70.0
    COMPONENTS
    unit_test_framework
    REQUIRED)


message(STATUS "[INFO] Building ICOV core unit tests")
set(SRC icovunittest.cc)
add_executable(icov_core_unit_tests ${SRC})
target_link_libraries(icov_core_unit_tests s_IcovCoreLib Boost::unit_test_framework)
# set the folder where to place the projects
set_target_properties(icov_core_unit_tests PROPERTIES FOLDER "public/tests" )

