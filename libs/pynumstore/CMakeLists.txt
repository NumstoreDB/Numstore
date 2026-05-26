find_package(Python 
  COMPONENTS Interpreter 
             Development.Module 
             Development.Embed 
  REQUIRED
)

# Force release python lib on Windows debug builds
if(WIN32 AND Python_LIBRARIES MATCHES "_d\\.lib")
    string(REPLACE "_d.lib" ".lib" Python_LIBRARIES "${Python_LIBRARIES}")
endif()

execute_process(COMMAND 
  "${Python_EXECUTABLE}" -c "import numpy as np; print(np.get_include())"
  OUTPUT_VARIABLE NumPy_INCLUDE_DIRS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

file(GLOB_RECURSE 
  NSPY_SOURCES 
  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/c/src/*.c"
)

Python_add_library(_pynumstore MODULE WITH_SOABI ${NSPY_SOURCES})

target_include_directories(_pynumstore
    PUBLIC          "${CMAKE_CURRENT_SOURCE_DIR}/c/include"
    PRIVATE         "${CMAKE_CURRENT_SOURCE_DIR}/c/src"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../nscore/include"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../c_specx/include"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../numstore/include"
)

# Include Numpy includes seperately to ignore compiler flags
target_include_directories(_pynumstore
    SYSTEM PRIVATE  "${NumPy_INCLUDE_DIRS}"
)

target_link_libraries(_pynumstore PRIVATE c_specx nscore numstore)

install(TARGETS _pynumstore DESTINATION pynumstore)
