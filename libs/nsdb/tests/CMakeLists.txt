list(
    APPEND NS_NSDB_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_read_var_page_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_visit_variables_tests.c
)

set(NS_NSDB_SRCS ${NS_NSDB_SRCS} PARENT_SCOPE)
