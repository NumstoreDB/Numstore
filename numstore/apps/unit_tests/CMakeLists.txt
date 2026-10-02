#####################################################################
### The unit test
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    message(STATUS "Python: ${Python3_EXECUTABLE}")

    set(_unit_tests_c ${CMAKE_CURRENT_BINARY_DIR}/unit_tests.c)

    add_custom_command(
        OUTPUT ${_unit_tests_c}
        COMMAND
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen_tests.py
            ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests.c.in ${_unit_tests_c}
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/gen_tests.py
            ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests.c.in
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating unit_tests.c"
        VERBATIM
    )

    add_executable(unit_tests ${_unit_tests_c})
    target_link_libraries(unit_tests PRIVATE numstore.library.private)
endif()
