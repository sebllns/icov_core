cmake_minimum_required(VERSION 3.10)


project(icov_core LANGUAGES CXX)

#add_compile_options(-Wall)

# library
set( LIB_NAME IcovCoreLib )


# get source files
file(GLOB_RECURSE SRC_FILES "*.cpp" )

# get include files
file(GLOB_RECURSE INC_FILES "*.h" "*.hpp" "*.hxx")


include_directories(. common/interactive_coder)


# library
add_library( ${LIB_NAME} SHARED ${SRC_FILES} ${INC_FILES})
add_library( s_${LIB_NAME} STATIC ${SRC_FILES} ${INC_FILES})

#install(TARGETS ${LIB_NAME} DESTINATION ./lib EXPORT icov-targets)
#install(EXPORT icov-targets DESTINATION lib/icov)

target_link_libraries(${LIB_NAME} aff3ct::aff3ct-static-lib ${Boost_LIBRARIES})
target_link_libraries(s_${LIB_NAME} aff3ct::aff3ct-static-lib ${Boost_LIBRARIES})
