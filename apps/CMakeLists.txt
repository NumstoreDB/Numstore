add_executable(ns_dlread ns_dlread.c)
target_link_libraries(ns_dlread PRIVATE numstore)

add_executable(ns_nspprint ns_nspprint.c)
target_link_libraries(ns_nspprint PRIVATE numstore)

add_executable(ns_nsspprint ns_nsspprint.c)
target_link_libraries(ns_nsspprint PRIVATE numstore)

add_executable(ns_simtest ns_simtest.c)
target_link_libraries(ns_simtest PRIVATE numstore)

add_executable(ns_walpprint ns_walpprint.c)
target_link_libraries(ns_walpprint PRIVATE numstore)

add_executable(print_all_vars print_all_vars.c)
target_link_libraries(print_all_vars PRIVATE numstore)

add_executable(print_query print_query.c)
target_link_libraries(print_query PRIVATE numstore)

add_executable(print_type print_type.c)
target_link_libraries(print_type PRIVATE numstore)

add_executable(resolve_type_ref resolve_type_ref.c)
target_link_libraries(resolve_type_ref PRIVATE numstore)

#####################################################################
### unit_tests
### gen_tests.py scrapes TEST(...) out of src/ and writes the main().
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    set(_unit_tests_c ${CMAKE_CURRENT_BINARY_DIR}/unit_tests.c)

    # Which os backend's tests to scrape. WIN32 is the target, not the host, so
    # this stays right under a mingw cross build.
    if(WIN32)
        set(_unit_tests_platform windows)
    else()
        set(_unit_tests_platform posix)
    endif()

    add_custom_command(
        OUTPUT ${_unit_tests_c}
        COMMAND
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen_tests.py
            ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests.c.in ${_unit_tests_c}
            ${_unit_tests_platform}
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/gen_tests.py
            ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests.c.in
            ${LIBNUMSTORE_SRCS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating unit_tests.c"
        VERBATIM
    )

    add_executable(unit_tests ${_unit_tests_c})
    target_link_libraries(unit_tests PRIVATE numstore)
endif()
