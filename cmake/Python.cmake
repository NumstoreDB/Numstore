# Look for python
find_package(Python COMPONENTS Interpreter Development REQUIRED)

function(run_python_script OUTPUT_VAR)
  execute_process(
    COMMAND "${Python_EXECUTABLE}" ${ARGN}
    OUTPUT_VARIABLE _output
    RESULT_VARIABLE _result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "Python script failed: ${_result}")
  endif()
  set(${OUTPUT_VAR} "${_output}" PARENT_SCOPE)
endfunction()

execute_process(
  COMMAND "${Python_EXECUTABLE}" 
          -c "import numpy as np; print(np.get_include())"
  OUTPUT_VARIABLE NumPy_INCLUDE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Numpy ${NumPy_INCLUDE}")
