if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_executable(ns_big_file ns_big_file.c)
    add_executable(ns_sample1_basic_crud ns_sample1_basic_crud.c)
    add_executable(smfile_sample1_basic_crud smfile_sample1_basic_crud.c)
    add_executable(smfile_sample2_transactions smfile_sample2_transactions.c)
    add_executable(smfile_sample3_stride smfile_sample3_stride.c)
    add_executable(
        smfile_sample4_rollback_commit
        smfile_sample4_rollback_commit.c
    )

    target_link_libraries(ns_big_file PRIVATE numstore.library)
    target_link_libraries(ns_sample1_basic_crud PRIVATE numstore.library)
    target_link_libraries(smfile_sample1_basic_crud PRIVATE numstore.library)
    target_link_libraries(smfile_sample2_transactions PRIVATE numstore.library)
    target_link_libraries(smfile_sample3_stride PRIVATE numstore.library)
    target_link_libraries(
        smfile_sample4_rollback_commit
        PRIVATE numstore.library
    )
endif()
