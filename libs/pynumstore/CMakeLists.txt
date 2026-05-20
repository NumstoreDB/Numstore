################### Dependencies
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

execute_process(
  COMMAND "${Python_EXECUTABLE}" -c "import numpy as np; print(np.get_include())"
  OUTPUT_VARIABLE NumPy_INCLUDE_DIRS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

################### Extension Module
file(GLOB_RECURSE NSPY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/c/src/*.c")

Python_add_library(_numstore MODULE ${NSPY_SOURCES})
target_include_directories(_numstore SYSTEM PRIVATE "${NumPy_INCLUDE_DIRS}")


target_include_directories(_numstore
    PUBLIC          "${CMAKE_CURRENT_SOURCE_DIR}/c/include"
    PRIVATE         "${CMAKE_CURRENT_SOURCE_DIR}/c/src"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../nscore/include"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../c_specx/include"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../numstore/include"
    SYSTEM PRIVATE  "${NumPy_INCLUDE_DIRS}"
)

target_link_libraries(_numstore PRIVATE c_specx nscore numstore)

################### Install
install(TARGETS _numstore DESTINATION pynumstore)
