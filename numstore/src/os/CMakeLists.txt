target_sources(
    numstore.library
    PRIVATE ns_faulty.c ns_faulty.h ns_os.h ns_posix.c ns_system.h
)

add_subdirectory(posix)
add_subdirectory(windows)
add_subdirectory(faulty)
