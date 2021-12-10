message("BUILD ICOV CORE UNIT TESTS")

#set(Boost_USE_STATIC_LIBS OFF)
#set(Boost_USE_MULTITHREADED ON)
#set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.70.0 COMPONENTS unit_test_framework REQUIRED)

set(SRC
    icovunittest.cc
    )

if(Boost_FOUND)
    add_executable(icov_core_unit_tests ${SRC})
    target_link_libraries(icov_core_unit_tests Boost::unit_test_framework s_IcovCoreLib)
endif()
