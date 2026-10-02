target_sources(
    numstore.library
    PRIVATE
        0001_create_delete_rollback_delete.c
        0002_create_crash_close_delete.c
        0003_rollback_invald_wal_header.c
)
