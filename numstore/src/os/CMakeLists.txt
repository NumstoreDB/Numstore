target_sources(
    numstore.library
    PRIVATE
        ns_file.h
        ns_filesystem.h
        ns_memory.h
        ns_os_vtable.h
        ns_threading.h
        ns_time.h
)

add_subdirectory(common)
add_subdirectory(posix)
add_subdirectory(test)
add_subdirectory(windows)
