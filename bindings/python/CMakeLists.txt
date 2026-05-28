# Look for python
find_package(Python 
  COMPONENTS Interpreter 
             Development.Module 
             Development.Embed 
  REQUIRED
)

# Import numpy includes
execute_process(COMMAND 
  "${Python_EXECUTABLE}" -c "import numpy as np; print(np.get_include())"
  OUTPUT_VARIABLE NumPy_INCLUDE_DIRS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# As more python packages get defined - I'll add conditional logic here
add_subdirectory(numstore)
