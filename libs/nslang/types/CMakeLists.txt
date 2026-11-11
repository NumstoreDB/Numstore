list(
    APPEND NS_TYPES_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_kvt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_query.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_sarray_t.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_struct_t.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_subtype.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_type_accessor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_type_ref.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_types.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_union_t.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_variables.c
)

set(NS_TYPES_SRCS ${NS_TYPES_SRCS} PARENT_SCOPE)
