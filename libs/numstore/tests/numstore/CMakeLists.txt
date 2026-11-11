list(
    APPEND NS_NUMSTORE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/0001_insert_txn.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0002_write_txn.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0003_create_txn.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0004_delete_txn.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0005_read_stride.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0006_write_stride.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0007_remove_stride.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0008_insert_positions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0009_struct_elements.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0010_query_api_mismatch.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0011_concurrent_separate_variables.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0012_concurrent_shared_variable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0013_concurrent_global_lock.c
)

set(NS_NUMSTORE_SRCS ${NS_NUMSTORE_SRCS} PARENT_SCOPE)
