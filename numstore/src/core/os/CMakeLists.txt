target_sources(numstore.library PRIVATE ns_malloc.c ns_malloc.h ns_os.h)

add_subdirectory(posix)
add_subdirectory(windows)
add_subdirectory(faulty)
