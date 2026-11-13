add_subdirectory(dpg_table)
add_subdirectory(lock_table)
add_subdirectory(pager)
add_subdirectory(txn_table)
add_subdirectory(wal)
add_subdirectory(recovery)

ns_add_library(ns_nscore ${NS_NSCORE_SRCS})
target_link_libraries(ns_nscore PUBLIC ns_disk_pager ns_disk_wal ns_page)
