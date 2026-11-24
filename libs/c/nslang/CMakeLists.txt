list(
    APPEND NS_TYPES_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_lexer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ns_tokens.c
)

add_subdirectory(parsers)
add_subdirectory(types)

ns_add_library(ns_types ${NS_TYPES_SRCS})
target_link_libraries(ns_types PUBLIC ns_page)
