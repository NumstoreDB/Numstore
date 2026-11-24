list(APPEND NS_NSDB_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/ns_nsdb.c)

add_subdirectory(tests)

ns_add_library(ns_nsdb ${NS_NSDB_SRCS})
target_link_libraries(ns_nsdb PUBLIC ns_algorithms)
