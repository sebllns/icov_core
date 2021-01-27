cmake_minimum_required(VERSION 3.5)

project(icov_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 

find_package(Boost 1.70.0 COMPONENTS unit_test_framework)

set(SRC
    icovunittest.cc
    )

set(HEADER
    #test/icovunittest.h
    )


if(Boost_FOUND)
    add_executable(icov_tests ${SRC} ${HEADER})
    target_link_libraries(icov_tests Boost::unit_test_framework)
endif()
