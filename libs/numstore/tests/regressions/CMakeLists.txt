list(
    APPEND NS_NUMSTORE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/0001_create_delete_rollback_delete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0002_create_crash_close_delete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0003_rollback_invald_wal_header.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0004_crash_after_churn_diverges.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0005_crash_twice_in_txn.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0006_empty_range_reads_to_end.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0007_open_with_error_not_terminated.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0008_delete_if_exists_ignored.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0009_get_if_exists_ignored.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0010_plan_call_after_failed_query.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0011_read_into_zero_length_buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0012_remove_empties_leftmost_page.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0013_rollback_delete_past_buffer_pool.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0014_type_size_overflow.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0015_crash_with_txn_open_across_checkpoint.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0016_crash_recovery_past_buffer_pool.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0017_smfile_write_across_end.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0018_smfile_stride_past_end.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0019_insert_short_source_buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0020_read_into_partial_element_buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0021_query_trailing_text.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0022_auto_txn_from_threads.c
)

set(NS_NUMSTORE_SRCS ${NS_NUMSTORE_SRCS} PARENT_SCOPE)
