################### Root
cmake_minimum_required(VERSION 3.18)
project(pynumstore_build)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

################### Dependencies
set(Python_FIND_VIRTUALENV STANDARD)
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# Get numpy includes
execute_process(
  COMMAND "${Python_EXECUTABLE}" -c "import numpy as np; print(np.get_include())"
  OUTPUT_VARIABLE NumPy_INCLUDE_DIRS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

################### C Library
file(GLOB_RECURSE NSPY_SOURCES  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/c/src/*.c")

################### THE MAIN LIBRARY
Python_add_library(cpynumstore STATIC ${NSPY_SOURCES})
target_include_directories(cpynumstore
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/c/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/c/src"
            "${CMAKE_SOURCE_DIR}/libs/nscore/include"
            "${CMAKE_SOURCE_DIR}/libs/c_specx/include"
            "${CMAKE_SOURCE_DIR}/libs/numstore/include"
            "${NumPy_INCLUDE_DIRS}"
)
target_link_libraries(cpynumstore PUBLIC c_specx nscore numstore) 

################### INSTALL
install(TARGETS cpynumstore DESTINATION pynumstore)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/pynumstore/
        DESTINATION pynumstore
        FILES_MATCHING PATTERN "*.py")

