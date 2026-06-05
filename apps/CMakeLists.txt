
############################## Add Executable
# Creates an executable target, sets up standard include directories, 
# and links it against the internal numstore library.
# This targets headers against all private and public includes
#
# FILE
#   The path to the primary source file (e.g., main.c)
#
# NAME
#   The target name for the generated executable
#
# Example:
#   ns_add_executable(src/main.c my_app)
#
function(ns_add_executable NAME FILE)
  add_executable(${NAME} ${FILE})
  target_include_directories(${NAME}
    PRIVATE "${CMAKE_SOURCE_DIR}/include"
            "${CMAKE_SOURCE_DIR}/src"
  )
  target_link_libraries(${NAME} PRIVATE numstore)
  install(TARGETS ${NAME} DESTINATION bin)
endfunction()

# Creates a single sample executable target, includes the project 
# header directory, and links it against the core library
# Only links public headers
#
# FILE
#   The path to the source file (e.g., sample.c).
#
# NAME
#   The target name for the generated executable.
#
function(ns_add_sample NAME FILE)
  add_executable(${NAME} ${FILE})
  target_include_directories(${NAME}
    PRIVATE "${CMAKE_SOURCE_DIR}/include"
            "${CMAKE_SOURCE_DIR}/src"
  )
  target_link_libraries(${NAME} PRIVATE numstore)
  install(TARGETS ${NAME} DESTINATION bin)
endfunction()

if(NOT ENABLE_NTEST) 
  ############################## Unit Test
  # See script/gen_tests.py which searches 
  # throughout the code base to find strings that 
  # match TEST (<NAME>) and runs the list of tests
  # 
  # Needs to generate that list first
  #
  execute_process(
      COMMAND "python3" "${CMAKE_SOURCE_DIR}/scripts/gen_tests.py"
              "${CMAKE_SOURCE_DIR}/templates/unit_tests.c.in"
              "${CMAKE_CURRENT_SOURCE_DIR}/unit_tests.c"
              "${CMAKE_SOURCE_DIR}/src"
      RESULT_VARIABLE GEN_RESULT
  )
  ns_add_executable(cgd_swarm_test cgd_swarm_test.c)
  ns_add_executable(irwr_swarm_test irwr_swarm_test.c)
  ns_add_executable(unit_tests unit_tests.c)
endif()

if(BUILD_TOOLS)
  ############################## Tools
  # Dev tools are utilities to diagnose 
  # bugs in numstore database 
  #
  # Simple tools that analyze datbaase 
  # issues and problems
  ns_add_executable(dlread dlread.c)
  ns_add_executable(nspprint nspprint.c)
  ns_add_executable(simple_nspprint simple_nspprint.c)
  ns_add_executable(walfprint walfprint.c)
endif()

if(BUILD_SAMPLES)
  ############################## Samples
  # Samples are pedagogical programs 
  # to help people new to the numstore ecosystem
  # 
  # Only include python scripts if we build python bindings
  #
  ns_add_sample(ns_sample1_basic_crud           ${CMAKE_CURRENT_SOURCE_DIR}/samples/numstore/sample1_basic_crud.c)
  ns_add_sample(smfile_sample1_basic_crud       ${CMAKE_CURRENT_SOURCE_DIR}/samples/smfile/sample1_basic_crud.c)
  ns_add_sample(smfile_sample2_transactions     ${CMAKE_CURRENT_SOURCE_DIR}/samples/smfile/sample2_transactions.c)
  ns_add_sample(smfile_sample3_stride           ${CMAKE_CURRENT_SOURCE_DIR}/samples/smfile/sample3_stride.c)
  ns_add_sample(smfile_sample4_variable_names   ${CMAKE_CURRENT_SOURCE_DIR}/samples/smfile/sample4_variable_names.c)
  ns_add_sample(smfile_sample5_rollback_commit  ${CMAKE_CURRENT_SOURCE_DIR}/samples/smfile/sample5_rollback_commit.c)

  if(BUILD_PYTHON_BINDINGS)
    install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/pynumstore/sample_1_basic_operations.py DESTINATION bin)
    install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/pynumstore/sample_2_transactions.py DESTINATION bin)
    install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/pynumstore/sample_3_multiple_variables.py DESTINATION bin)
    install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/pynumstore/sample_4_overwriting.py DESTINATION bin)
    install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/pynumstore/sample_5_close_reopen.py DESTINATION bin)
  endif()
endif()
