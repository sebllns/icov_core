cmake_minimum_required(VERSION 3.10)

#add_compile_options(-Wall)

# library
set( LIB_NAME IcovCoreLib )

# get source files
file(GLOB_RECURSE SRC_FILES "common/*.cpp" "io/*.cpp" "ldpc_codec/*.cpp" "omnidirectional_layer/*.cpp")
#"video_layer/*.cpp"

# get include files
#file(GLOB_RECURSE INC_FILES "common/*.h" "common/*.hpp" "common/*.hxx")

message(WARNING "make static analysis optional")
#set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=-*,clang-analyzer-*,boost-*,performance-*,cppcoreguidelines-owning-memory,portability-*")
#,readability-*

# library
add_library( ${LIB_NAME} SHARED ${SRC_FILES})
add_library( s_${LIB_NAME} STATIC ${SRC_FILES})
#add_dependencies(${LIB_NAME} aff3ct_project)
#add_dependencies(s_${LIB_NAME} aff3ct_project)

#install(TARGETS ${LIB_NAME} DESTINATION ./lib EXPORT icov-targets)
#install(EXPORT icov-targets DESTINATION lib/icov)


target_include_directories(${LIB_NAME} PUBLIC . common common/dev_tools ldpc_codec omnidirectional_layer video_layer)
target_include_directories(s_${LIB_NAME} PUBLIC . common common/dev_tools ldpc_codec omnidirectional_layer video_layer)

target_link_libraries(${LIB_NAME} aff3ct::aff3ct-static-lib ${Boost_LIBRARIES} Threads::Threads ${CMAKE_DL_LIBS} CommonLib)
target_link_libraries(s_${LIB_NAME} aff3ct::aff3ct-static-lib ${Boost_LIBRARIES} Threads::Threads ${CMAKE_DL_LIBS} CommonLib)

# set the folder where to place the projects
set_target_properties( ${LIB_NAME} PROPERTIES FOLDER "public/lib/shared" )
set_target_properties( s_${LIB_NAME} PROPERTIES FOLDER "public/lib/static" )