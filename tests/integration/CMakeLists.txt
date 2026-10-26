find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_test(NAME roundtrip
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/roundtrip.sh
                 $<TARGET_FILE_DIR:icov_cli> ${Python3_EXECUTABLE}
                 ${PROJECT_SOURCE_DIR}/scripts ${CMAKE_CURRENT_BINARY_DIR}/work)
set_tests_properties(roundtrip PROPERTIES LABELS integration TIMEOUT 600)
