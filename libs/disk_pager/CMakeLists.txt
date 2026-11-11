list(APPEND NS_DISK_PAGER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/ns_file_pager.c)

ns_add_library(ns_disk_pager ${NS_DISK_PAGER_SRCS})
target_link_libraries(ns_disk_pager PUBLIC ns_common)
