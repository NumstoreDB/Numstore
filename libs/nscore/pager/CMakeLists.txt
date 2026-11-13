list(
    APPEND NS_NSCORE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_page_fixture.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_begin_txn.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_cancel.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_checkpoint.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_close.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_commit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_crash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_delete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_evict.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_flush.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_get.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_get_writable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_logging.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_new.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_open.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_release.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_reserve.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_rollback.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_unfix.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_write_header.c
)

add_subdirectory(tests)

set(NS_NSCORE_SRCS ${NS_NSCORE_SRCS} PARENT_SCOPE)
