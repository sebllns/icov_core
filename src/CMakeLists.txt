cmake_minimum_required(VERSION 3.10)


project(icov_core LANGUAGES CXX)

#add_compile_options(-Wall)

# library
set( LIB_NAME IcovCoreLib )

include_directories(. common common/dev_tools ldpc_codec omnidirectional_layer video_layer)

# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp" )

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")

set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=-*,clang-analyzer-*,boost-*,performance-*,cppcoreguidelines-owning-memory,portability-*")
#,readability-*

# library
add_library( ${LIB_NAME} SHARED ${SRC_FILES} ${INC_FILES})
add_library( s_${LIB_NAME} STATIC ${SRC_FILES} ${INC_FILES})

#install(TARGETS ${LIB_NAME} DESTINATION ./lib EXPORT icov-targets)
#install(EXPORT icov-targets DESTINATION lib/icov)

target_link_libraries(${LIB_NAME} aff3ct-static-lib ${Boost_LIBRARIES})
target_link_libraries(s_${LIB_NAME} aff3ct-static-lib ${Boost_LIBRARIES})
