add_subdirectory(numstore)
add_subdirectory(regressions)
add_subdirectory(simulation)

target_sources(numstore.library PRIVATE ns_aries_tests.c)
