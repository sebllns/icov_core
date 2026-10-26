########################################
#                CORE                  #
########################################
# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp")

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")

# default LDPC matrix set and q-ary table, compiled in (see common/icov_data.h)
include(${PROJECT_SOURCE_DIR}/cmake/EmbedData.cmake)
set(EMBEDDED_DATA ${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_data.cpp)
icov_embed_data(${EMBEDDED_DATA} ${PROJECT_SOURCE_DIR}/data imt_164_v2 q-ary)

add_library(IcovCoreLib ${SRC_FILES} ${INC_FILES} ${EMBEDDED_DATA})
target_include_directories(IcovCoreLib PUBLIC . common)
target_link_libraries(IcovCoreLib PUBLIC ${Boost_LIBRARIES} ${Boost_LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} ${CMAKE_DL_LIBS})
if(ICOV_USE_OMP)
    add_compile_definitions(ICOV_USE_OMP)
    target_link_libraries(IcovCoreLib PUBLIC OpenMP::OpenMP_CXX)
endif()
set_target_properties(IcovCoreLib PROPERTIES FOLDER "public/lib")
