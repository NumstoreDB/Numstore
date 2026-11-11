list(
    APPEND NS_PAGE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_page.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_page_data_list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_page_fsm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_page_inner_node.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_page_var_hash_page.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_page_var_page.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_page_var_tail.c
)

ns_add_library(ns_page ${NS_PAGE_SRCS})
target_link_libraries(ns_page PUBLIC ns_common)
