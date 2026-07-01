set(NS_VERSION_MAJOR  ${PROJECT_VERSION_MAJOR})
set(NS_VERSION_MINOR  ${PROJECT_VERSION_MINOR})
set(NS_VERSION_PATCH  ${PROJECT_VERSION_PATCH})
set(NS_VERSION_STRING "${PROJECT_VERSION}")

########################################################################
# Generate the numstore library
########################################################################
file(GLOB NUMSTORE_SOURCES CONFIGURE_DEPENDS "*.c")
file(GLOB NUMSTORE_TESTING_SOURCES CONFIGURE_DEPENDS "testing/*.c")

if(ENABLE_TESTS)
    add_library(numstore ${NUMSTORE_SOURCES} ${NUMSTORE_TESTING_SOURCES})
else()
    add_library(numstore ${NUMSTORE_SOURCES})
endif()
add_library(numstore::numstore ALIAS numstore)

# PRIVATE is key: this must NOT propagate to targets that link against numstore,
# otherwise main() would get stripped out of the executable build below too.
target_compile_definitions(numstore PRIVATE NUMSTORE_LIB)

set_target_properties(numstore PROPERTIES VERSION ${PROJECT_VERSION})
if(NOT WIN32)
    set_target_properties(numstore PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
endif()
target_include_directories(numstore PRIVATE "${PROJECT_SOURCE_DIR}/src")

if(WIN32)
    target_link_libraries(numstore PUBLIC ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(numstore PUBLIC Threads::Threads)
endif()

########################################################################
# Generate the numstore executable
########################################################################
# Recompile numstore.c a second time (this time NUMSTORE_LIB is NOT
# defined, so main() is included), and link against the library for
# everything else.
add_executable(numstore_exe "${CMAKE_CURRENT_SOURCE_DIR}/numstore.c")
set_target_properties(numstore_exe PROPERTIES OUTPUT_NAME numstore)
target_include_directories(numstore_exe PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(numstore_exe PRIVATE numstore::numstore)

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
            "${PROJECT_SOURCE_DIR}/src/scripts/gen_tests.py"
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

  ns_add_executable(unit_tests        testing/apps/unit_tests.c)
  ns_add_executable(cgd_swarm_test    testing/apps/cgd_swarm_test.c)
  ns_add_executable(irwr_swarm_test   testing/apps/irwr_swarm_test.c)
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
