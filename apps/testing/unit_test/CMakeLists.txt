############################## Generate Test Suite

# Generate a list of test names
run_python_script(TEST_NAMES
    "${CMAKE_SOURCE_DIR}/scripts/gen_tests.py"
    "${CMAKE_SOURCE_DIR}/libs"
    "${CMAKE_SOURCE_DIR}/bindings/python/numstore/c"
)

# Check the output - should be something
if(TEST_NAMES STREQUAL "")
    message(FATAL_ERROR "No output from gen_tests.py")
endif()

# Replace new lines with semi colons
string(REPLACE "\n" ";" TEST_NAMES_LIST "${TEST_NAMES}")

# Build output string and count the number of tests
set(PYTHON_OUTPUT "")
set(COUNT 0)
foreach(TEST_NAME ${TEST_NAMES_LIST})
    string(APPEND PYTHON_OUTPUT "  REGISTER (smartfiles, ${TEST_NAME});\n")
    math(EXPR COUNT "${COUNT} + 1")
endforeach()

# Read in the test template
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/register_test_suite.h.in" FILE_CONTENTS)

# Replace variables
string(REPLACE "%NAME%"  "smartfiles"       FILE_CONTENTS "${FILE_CONTENTS}")
string(REPLACE "%TESTS%" "${PYTHON_OUTPUT}" FILE_CONTENTS "${FILE_CONTENTS}")
string(REPLACE "%COUNT%" "${COUNT}"         FILE_CONTENTS "${FILE_CONTENTS}")

# Write to new file
file(
  WRITE "${CMAKE_CURRENT_SOURCE_DIR}/register_test_suite.h" "${FILE_CONTENTS}"
)

############################## Unit Test Executable
file(GLOB_RECURSE NSPY_SOURCES 
    CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/bindings/python/numstore/c/src/*.c"
)

add_executable(unit_test main.c "${NSPY_SOURCES}")

target_link_libraries(unit_test
    PRIVATE c_specx 
            nscore 
            smartfiles 
            numstore 
            nstesting 
            Python::Python
)

# system include to avoid pedantic flags for numpy stuff
target_include_directories(unit_test SYSTEM PRIVATE "${NumPy_INCLUDE}")
target_include_directories(unit_test
    PRIVATE "${CMAKE_SOURCE_DIR}/libs/c_specx/include"
            "${CMAKE_SOURCE_DIR}/bindings/python/numstore/c/include"
            "${CMAKE_SOURCE_DIR}/bindings/python/numstore/c/src"
            "${CMAKE_SOURCE_DIR}/libs/numstore/src"
)

if(MSVC)
    set_property(TARGET unit_test
      PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    )
endif()

# The test need to import _numstore 
# do that by defining this then running PyImport
target_compile_definitions(unit_test PRIVATE
  NUMSTORE_SO_DIR="${CMAKE_BINARY_DIR}/bin"
)

############################## CTest
# Add a leak checker tool if asan is off
if(ENABLE_LEAK_TOOL AND NOT ENABLE_ASAN)
  set(MEMORYCHECK_SUPPRESSIONS_FILE 
    "${CMAKE_SOURCE_DIR}/valgrind_suppress.txt")
  set(MEMORYCHECK_COMMAND_OPTIONS
      "--tool=memcheck \
       --leak-check=full \
       --leak-resolution=high \
       --show-leak-kinds=all \
       --track-origins=yes \
       --track-fds=yes \
       --error-exitcode=1 \
       --errors-for-leak-kinds=all \
       --undef-value-errors=yes \
       --partial-loads-ok=no \
       --expensive-definedness-checks=yes \
       --show-mismatched-frees=yes \
       --show-reachable=yes \
       --num-callers=40"
  )
endif()

include(CTest)
enable_testing()
add_test(NAME unit_test COMMAND unit_test)

if(ENABLE_ASAN)
    set_tests_properties(unit_test PROPERTIES
        ENVIRONMENT
        # Suppress address sanitizer for python
        # - python and numpy don't clean their memory
        "LSAN_OPTIONS=suppressions=${CMAKE_SOURCE_DIR}/asan_suppress.txt"
    )
endif()
