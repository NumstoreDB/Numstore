list(
    APPEND LIBNUMSTORE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/0001_pager_is_thread_safe_inside_a_single_txn.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0002_pager_is_thread_safe_across_txns.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0003_pager_is_thread_safe_on_single_page.c
    ${CMAKE_CURRENT_SOURCE_DIR}/0004_pager_new_get.c
)

set(LIBNUMSTORE_SRCS ${LIBNUMSTORE_SRCS} PARENT_SCOPE)
