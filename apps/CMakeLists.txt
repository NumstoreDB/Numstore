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
  target_include_directories(${NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include")
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
              "${CMAKE_CURRENT_SOURCE_DIR}/testing/unit_tests.c"
              "${CMAKE_SOURCE_DIR}/src"
      RESULT_VARIABLE GEN_RESULT
  )
  ns_add_executable(cgd_swarm_test testing/cgd_swarm_test.c)
  ns_add_executable(irwr_swarm_test testing/irwr_swarm_test.c)
  ns_add_executable(unit_tests testing/unit_tests.c)
endif()

if(BUILD_TOOLS)
  ############################## Tools
  # Dev tools are utilities to diagnose 
  # bugs in numstore database 
  #
  # Simple tools that analyze datbaase 
  # issues and problems
  ns_add_executable(dlread tools/dlread.c)
  ns_add_executable(nspprint tools/nspprint.c)
  ns_add_executable(simple_nspprint tools/simple_nspprint.c)
  ns_add_executable(walfprint tools/walfprint.c)
endif()

if(BUILD_SAMPLES)
  ############################## Samples
  # Samples are pedagogical programs 
  # to help people new to the numstore ecosystem
  #
  ns_add_sample(ns_sample1_basic_crud           samples/numstore/sample1_basic_crud.c)
  ns_add_sample(smfile_sample1_basic_crud       samples/smartfiles/sample1_basic_crud.c)
  ns_add_sample(smfile_sample2_transactions     samples/smartfiles/sample2_transactions.c)
  ns_add_sample(smfile_sample3_stride           samples/smartfiles/sample3_stride.c)
  ns_add_sample(smfile_sample4_variable_names   samples/smartfiles/sample4_variable_names.c)
  ns_add_sample(smfile_sample5_rollback_commit  samples/smartfiles/sample5_rollback_commit.c)
endif()

if(BUILD_APP)
  ns_add_executable(nscli numstore.c)
endif()
