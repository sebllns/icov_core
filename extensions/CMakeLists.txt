cmake_minimum_required(VERSION 3.10)

message(STATUS "ICOV EXTENSIONS ENABLED")


#add_compile_options(-Wall)


set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.45.0) 

if(ALL_EXTENSIONS_ENABLED)

if(Boost_FOUND)
    set(BITSTREAM_UTILS_ENABLED ON)
endif()

endif()


if(BITSTREAM_UTILS_ENABLED)
    add_subdirectory(bitstream-utils)
endif()
