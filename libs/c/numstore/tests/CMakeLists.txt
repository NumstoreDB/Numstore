list(APPEND NS_NUMSTORE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/ns_aries_tests.c)

add_subdirectory(numstore)
add_subdirectory(regressions)
add_subdirectory(simulation)

set(NS_NUMSTORE_SRCS ${NS_NUMSTORE_SRCS} PARENT_SCOPE)
