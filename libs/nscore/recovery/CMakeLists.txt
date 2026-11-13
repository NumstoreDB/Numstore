list(
    APPEND NS_NSCORE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_aries_ctx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_detect_wal_and_recover.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_recover.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_restart_analysis.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_restart_redo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_pgr_restart_undo.c
)

set(NS_NSCORE_SRCS ${NS_NSCORE_SRCS} PARENT_SCOPE)
