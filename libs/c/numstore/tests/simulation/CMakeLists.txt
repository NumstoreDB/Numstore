list(
    APPEND NS_NUMSTORE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_db_state_machine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_mem_vhmap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_numstore_simulation.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_operation_generator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_ref_state_machine.c
)

set(NS_NUMSTORE_SRCS ${NS_NUMSTORE_SRCS} PARENT_SCOPE)
