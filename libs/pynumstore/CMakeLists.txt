#####################################################################
### pynumstore
### The CPython extension. Links numstore; nothing links back.
find_package(
    Python
    3.9
    REQUIRED
    COMPONENTS Interpreter Development.Module NumPy
)

python_add_library(
    _pynumstore
    MODULE
    WITH_SOABI
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pynumstore/ns_pynumstore.c
)
target_link_libraries(_pynumstore PRIVATE numstore::numstore Python::NumPy)
set_target_properties(_pynumstore PROPERTIES C_VISIBILITY_PRESET hidden)

# Lands next to the Python sources in the wheel - see wheel.packages.
install(TARGETS _pynumstore LIBRARY DESTINATION pynumstore COMPONENT python)
