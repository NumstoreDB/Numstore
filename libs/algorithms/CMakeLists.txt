list(APPEND NS_ALGORITHMS_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/ns_node_updates.c)

add_subdirectory(numstore)
add_subdirectory(rope)
add_subdirectory(smartfiles)
add_subdirectory(var)

ns_add_library(ns_algorithms ${NS_ALGORITHMS_SRCS})
target_link_libraries(ns_algorithms PUBLIC ns_common ns_types ns_page ns_nscore)
