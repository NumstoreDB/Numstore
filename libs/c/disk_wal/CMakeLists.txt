list(
    APPEND NS_DISK_WAL_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_wal_istream.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_wal_ostream.c
)

ns_add_library(ns_disk_wal ${NS_DISK_WAL_SRCS})
target_link_libraries(ns_disk_wal PUBLIC ns_common)
