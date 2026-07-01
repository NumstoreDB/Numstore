############################## Add Numstore Executable
# Creates an executable target, sets up standard include directories,
# links it against the internal numstore library, and installs it.
# This targets headers against all private and public includes.
#
# NAME
#   The target name for the generated executable
#
# FILE
#   The path to the primary source file (e.g., main.c)
#
# Example:
#   ns_add_executable(my_app src/main.c)
#
function(ns_add_executable NAME FILE)
  add_executable(${NAME} ${FILE})
  target_include_directories(${NAME}
    PRIVATE "${PROJECT_SOURCE_DIR}/src"
  )
  target_link_libraries(${NAME} PRIVATE numstore::numstore)
  install(TARGETS ${NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

############################## Add Numstore Sample
# Creates a single sample executable target, includes the project
# header directory, and links it against the core library.
# Only links public headers.
#
# NAME
#   The target name for the generated executable.
#
# FILE
#   The path to the source file (e.g., sample.c).
#
function(ns_add_sample NAME FILE)
  add_executable(${NAME} ${FILE})
  target_include_directories(${NAME} PRIVATE "${PROJECT_SOURCE_DIR}/src")
  target_link_libraries(${NAME} PRIVATE numstore::numstore)
  install(TARGETS ${NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

########################################################################
# Testing Code
########################################################################
if(ENABLE_TESTS)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)

  # Generate unit test header
  execute_process(
    COMMAND ${Python3_EXECUTABLE}
            "${PROJECT_SOURCE_DIR}/src/apps/scripts/gen_tests.py"
            "${PROJECT_SOURCE_DIR}/src/templates/unit_tests.h.in"
            "${PROJECT_SOURCE_DIR}/src/testing/unit_tests.h"
            "${PROJECT_SOURCE_DIR}/src"
    RESULT_VARIABLE gen_result
    OUTPUT_VARIABLE gen_stdout
    ERROR_VARIABLE  gen_stderr
  )
  if(NOT gen_result EQUAL 0)
    message(FATAL_ERROR "gen_tests.py failed (${gen_result}):\n${gen_stdout}\n${gen_stderr}")
  endif()

  ns_add_executable(unit_tests        testing/unit_tests.c)
  ns_add_executable(cgd_swarm_test    testing/cgd_swarm_test.c)
  ns_add_executable(irwr_swarm_test   testing/irwr_swarm_test.c)
endif()

########################################################################
# Tools — simple UNIX-like debug tools 
########################################################################
if(BUILD_TOOLS)
  ns_add_executable(dlread          tools/dlread.c)
  ns_add_executable(nspprint        tools/nspprint.c)
  ns_add_executable(simple_nspprint tools/simple_nspprint.c)
  ns_add_executable(walfprint       tools/walfprint.c)
endif()

########################################################################
# Samples — pedagogical programs for numstore newcomers
########################################################################
if(BUILD_SAMPLES)
  ns_add_sample(ns_sample1_basic_crud          samples/sample1_basic_crud.c)
endif()

ns_add_executable(nscli numstore.c)
