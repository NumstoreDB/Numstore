############################## Set up python
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

############################## Find Numpy
execute_process(
  COMMAND "${Python_EXECUTABLE}" 
          -c "import numpy as np; print(np.get_include())"
  OUTPUT_VARIABLE NumPy_INCLUDE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Found Numpy: ${NumPy_INCLUDE}")
file(GLOB
  NSPY_SOURCES 
  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/c/src/*.c"
)

############################## Set up python library
Python_add_library(_pynumstore MODULE WITH_SOABI c/module.c ${NSPY_SOURCES})
target_include_directories(_pynumstore
    PRIVATE "${CMAKE_SOURCE_DIR}/include"
            "${CMAKE_SOURCE_DIR}/src"
            "${CMAKE_CURRENT_SOURCE_DIR}/c/include/"
            "${CMAKE_CURRENT_SOURCE_DIR}/c/src/"
)

# Include Numpy includes seperately to ignore compiler flags
target_include_directories(_pynumstore
    SYSTEM PRIVATE  "${NumPy_INCLUDE}"
)

target_link_libraries(_pynumstore PRIVATE numstore)
install(TARGETS _pynumstore DESTINATION pynumstore)
