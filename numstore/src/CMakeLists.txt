add_subdirectory(os)
add_subdirectory(core)
add_subdirectory(nscore)

target_sources(numstore.library PRIVATE numstore.c)
