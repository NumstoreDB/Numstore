################### Dependencies
find_package(Python COMPONENTS Interpreter Development.Module Development.Embed REQUIRED)

execute_process(
  COMMAND "${Python_EXECUTABLE}" -c "import numpy as np; print(np.get_include())"
  OUTPUT_VARIABLE NumPy_INCLUDE_DIRS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

################### Extension Module
file(GLOB_RECURSE NSPY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/c/src/*.c")

Python_add_library(_pynumstore MODULE ${NSPY_SOURCES})
target_include_directories(_pynumstore SYSTEM PRIVATE "${NumPy_INCLUDE_DIRS}")


target_include_directories(_pynumstore
    PUBLIC          "${CMAKE_CURRENT_SOURCE_DIR}/c/include"
    PRIVATE         "${CMAKE_CURRENT_SOURCE_DIR}/c/src"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../nscore/include"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../c_specx/include"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../numstore/include"
    SYSTEM PRIVATE  "${NumPy_INCLUDE_DIRS}"
)

target_link_libraries(_pynumstore PRIVATE c_specx nscore numstore)
target_compile_definitions(_pynumstore PRIVATE NTEST)

################### Install
install(TARGETS _pynumstore DESTINATION pynumstore)

################### Tests (only when building the project directly, not via pip)
option(PYNUMSTORE_BUILD_TESTS "Build pynumstore C test binary" OFF)

if(PYNUMSTORE_BUILD_TESTS)
    enable_testing()

    add_executable(pynumstore_test
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/numpy_api.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/c/src/ns_compile_type.c"
    )

    target_compile_definitions(pynumstore_test PRIVATE
        "PYNUMSTORE_PYPATH=\"${CMAKE_CURRENT_SOURCE_DIR}\""
    )

    target_include_directories(pynumstore_test
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/c/include"
                "${CMAKE_CURRENT_SOURCE_DIR}/../nscore/include"
                "${CMAKE_CURRENT_SOURCE_DIR}/../c_specx/include"
                "${CMAKE_CURRENT_SOURCE_DIR}/../numstore/include"
        SYSTEM PRIVATE "${NumPy_INCLUDE_DIRS}"
    )

    # numpy headers are not pedantic-clean; suppress the error for this target only
    target_compile_options(pynumstore_test PRIVATE -Wno-pedantic)

    target_link_libraries(pynumstore_test PRIVATE Python::Python c_specx nscore numstore)

    add_test(NAME pynumstore_c_tests COMMAND pynumstore_test)
    set_tests_properties(pynumstore_c_tests PROPERTIES
        ENVIRONMENT "LSAN_OPTIONS=detect_leaks=0"
    )

    add_test(
        NAME pynumstore_pytest
        COMMAND ${Python_EXECUTABLE} -m pytest "${CMAKE_CURRENT_SOURCE_DIR}/tests/"
                --ignore="${CMAKE_CURRENT_SOURCE_DIR}/tests/test.c"
                --ignore="${CMAKE_CURRENT_SOURCE_DIR}/tests/numpy_api.c"
                --tb=short -q
    )
endif()
