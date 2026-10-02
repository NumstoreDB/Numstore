target_sources(
    numstore.library
    PRIVATE ns_compiler.h ns_lexer.c ns_lexer.h ns_tokens.c ns_tokens.h
)

add_subdirectory(parsers)
