list(
    APPEND NS_NSCORE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/pager_new_get_concurrency.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pgr_multi_txns_concurrency.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pgr_single_page_concurrency.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pgr_single_txn_concurrency.c
)

set(NS_NSCORE_SRCS ${NS_NSCORE_SRCS} PARENT_SCOPE)
