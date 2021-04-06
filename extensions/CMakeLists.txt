cmake_minimum_required(VERSION 3.10)

#add_compile_options(-Wall)


if(ALL_EXTENSIONS_ENABLED OR BITSTREAM_UTILS_ENABLED)
    add_subdirectory(bitstream-utils)
endif()
