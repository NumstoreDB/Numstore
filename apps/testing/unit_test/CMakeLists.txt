############################## Generate Test Suite

## Find all unit tests
execute_process(
  COMMAND python3 
          "${CMAKE_SOURCE_DIR}/scripts/gen_tests.py" 
          "smartfiles" 
          "${CMAKE_SOURCE_DIR}/libs"
  OUTPUT_VARIABLE PYTHON_OUTPUT
  RESULT_VARIABLE PYTHON_RESULT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT PYTHON_RESULT EQUAL 0)
    message(FATAL_ERROR "Python script failed to execute: ${PYTHON_RESULT}")
endif()

# Count the lines of the output
if(PYTHON_OUTPUT STREQUAL "")
  message(FATAL_ERROR "No output from gen tests command, something went wrong")
else()
  string(REGEX MATCHALL "\n" NEWLINES "${PYTHON_OUTPUT}")
  list(LENGTH NEWLINES NEWLINE_COUNT)
  math(EXPR COUNT "${NEWLINE_COUNT} + 1")
endif()

# Read in the template file
file(
  READ "${CMAKE_CURRENT_SOURCE_DIR}/register_test_suite.h.in" 
  FILE_CONTENTS
)

# Make substitutions
string(REPLACE "%NAME%" "smartfiles" FILE_CONTENTS "${FILE_CONTENTS}")
string(REPLACE "%TESTS%" "${PYTHON_OUTPUT}" FILE_CONTENTS "${FILE_CONTENTS}")
string(REPLACE "%COUNT%" "${COUNT}" FILE_CONTENTS "${FILE_CONTENTS}")

# Write to the output file
file(
  WRITE "${CMAKE_CURRENT_SOURCE_DIR}/register_test_suite.h" 
  "${FILE_CONTENTS}"
)

############################## Add unit test exe
add_executable(unit_test "${CMAKE_CURRENT_SOURCE_DIR}/main.c")

target_link_libraries(unit_test
    PRIVATE c_specx 
            nscore 
            smartfiles 
            numstore
)

target_include_directories(unit_test
  PRIVATE "${CMAKE_SOURCE_DIR}/libs/c_specx/include"
          "${CMAKE_SOURCE_DIR}/libs/nscore/include"
          "${CMAKE_SOURCE_DIR}/libs/smartfiles/include"
)

