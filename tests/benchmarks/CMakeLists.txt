message("BUILD ICOV INTERACTIVE CODER TEST PROJECT")

# get source files
file(GLOB SRC_FILES "*.cpp")

# get include files
file(GLOB INC_FILES "*.h")

include_directories(bitvector)

#target_link_libraries(${EXE_NAME}  s_IcovRendererLib s_IcovNetworkLib s_IcovCoreLib aff3ct::aff3ct-static-lib)

if(Boost_FOUND)
    add_executable(bitvector_benchmark ${SRC} ${HEADER} bitvector/main.cpp)
    target_link_libraries(bitvector_benchmark s_IcovCoreLib)
endif()
