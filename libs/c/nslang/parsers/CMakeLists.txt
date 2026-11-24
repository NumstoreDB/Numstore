list(
    APPEND NS_TYPES_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_parse_multi_user_stride.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_parse_query.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_parse_subtype.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_parse_type.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_parse_type_ref.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_parse_user_stride.c
)

set(NS_TYPES_SRCS ${NS_TYPES_SRCS} PARENT_SCOPE)
