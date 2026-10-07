cmake_minimum_required(VERSION 3.20)
project(numstore_python LANGUAGES C CXX)

# Add numstore as a dependency. An sdist carries the library at _numstore/,
# put there by the Makefile's vendor target; a git checkout has no copy and
# uses the library in the tree. Override either with -DNUMSTORE_DIR=<dir>.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/_numstore/CMakeLists.txt)
    set(_numstore_default ${CMAKE_CURRENT_SOURCE_DIR}/_numstore)
else()
    set(_numstore_default ${CMAKE_CURRENT_SOURCE_DIR}/../../numstore)
endif()

set(
    NUMSTORE_DIR
    ${_numstore_default}
    CACHE PATH
    "Path to the numstore C library sources"
)
add_subdirectory(${NUMSTORE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/numstore)

find_package(
    Python
    3.9
    REQUIRED
    COMPONENTS Interpreter Development.Module NumPy
)

python_add_library(_pynumstore MODULE WITH_SOABI src/c/ns_pynumstore.c)
target_include_directories(_pynumstore PRIVATE src/c)
target_link_libraries(
    _pynumstore
    PRIVATE
        numstore.library.private # numstore + include/ + src/
        Python::NumPy # replaces np.get_include()
)
set_target_properties(_pynumstore PROPERTIES C_VISIBILITY_PRESET hidden)

install(TARGETS _pynumstore LIBRARY DESTINATION pynumstore COMPONENT python)
