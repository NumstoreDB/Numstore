add_subdirectory(numstore)
add_subdirectory(rope)
add_subdirectory(smartfiles)
add_subdirectory(var)

target_sources(numstore.library PRIVATE ns_node_updates.c ns_node_updates.h)
