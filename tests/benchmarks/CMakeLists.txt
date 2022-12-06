message("BUILD ICOV INTERACTIVE CODER TEST PROJECT")

# get source files
file(GLOB SRC_FILES "*.cpp")

# get include files
file(GLOB INC_FILES "*.h")

include_directories(bitvector)

#target_link_libraries(${EXE_NAME}  IcovRendererLib IcovNetworkLib IcovCoreLib aff3ct::aff3ct-static-lib)

add_executable(bitvector_benchmark ${SRC} ${HEADER} bitvector/main.cpp)
target_link_libraries(bitvector_benchmark IcovCoreLib aff3ct::aff3ct-static-lib)
# set the folder where to place the projects
set_target_properties( bitvector_benchmark PROPERTIES FOLDER "public/tests" )



