list(
    APPEND NS_COMMON_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_malloc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_os.c
)

add_subdirectory(faulty)
add_subdirectory(posix)
add_subdirectory(windows)

set(NS_COMMON_SRCS ${NS_COMMON_SRCS} PARENT_SCOPE)
