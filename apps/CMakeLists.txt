############################## Generate unit_test.c
 
execute_process(
    COMMAND "python3" "${CMAKE_SOURCE_DIR}/scripts/gen_tests.py"
            "${CMAKE_SOURCE_DIR}/templates/unit_tests.c.in"
            "${CMAKE_CURRENT_SOURCE_DIR}/unit_tests.c"
            "${CMAKE_SOURCE_DIR}/src"
    RESULT_VARIABLE GEN_RESULT
)

if(NOT GEN_RESULT EQUAL 0)
    message(FATAL_ERROR "gen_tests.py failed")
endif()
