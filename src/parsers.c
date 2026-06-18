
#include "alloc.h"
#include "collections.h"
#include "compiler.h"
#include "error.h"
#include "query.h"
#include "testing/testing.h"

/******************************************************************************
 * SECTION: Multi User Stride
 * ----------------------------------------------------------------------------
 * multi_user_stride ::= '[' ( entry ( ',' entry )* )? ']'
 *
 * entry             ::= INTEGER ( ':' INTEGER? ( ':' INTEGER? )? )? |
 *                       ':' INTEGER? ( ':' INTEGER? )?
 ******************************************************************************/

struct multi_user_stride_parser
{
  struct parser      *base;
  struct mus_builder  builder;
  struct chunk_alloc  temp;
  struct chunk_alloc *persistent;
};

// Parse optional ':' NUMBER (step)
static err_t
parse_mus_step (
    struct multi_user_stride_parser *parser,
    struct user_stride              *s,
    error                           *e
)
{
  if (!parser_match (parser->base, TT_COLON))
  {
    return SUCCESS;
  }

  s->present |= COLON_PRESENT;
  parser_advance (parser->base);

  if (parser_match (parser->base, TT_INTEGER))
  {
    struct token *tok = parser_advance (parser->base);
    s->step           = (sb_size)tok->integer;
    s->present |= STEP_PRESENT;
  }

  return SUCCESS;
}

static err_t
parse_mus_stop (
    struct multi_user_stride_parser *parser,
    struct user_stride              *s,
    error                           *e
)
{
  if (parser_match (parser->base, TT_INTEGER))
  {
    struct token *tok = parser_advance (parser->base);
    s->stop           = (sb_size)tok->integer;
    s->present |= STOP_PRESENT;
  }

  return parse_mus_step (parser, s, e);
}

static err_t
parse_entry (struct multi_user_stride_parser *parser, error *e)
{
  struct user_stride s = {0};

  // Optional start integer
  if (parser_match (parser->base, TT_INTEGER))
  {
    struct token *tok = parser_advance (parser->base);
    s.start           = (sb_size)tok->integer;
    s.present |= START_PRESENT;

    // Bare number with no colon → single index
    if (!parser_match (parser->base, TT_COLON))
    {
      return musb_accept_key (&parser->builder, s, e);
    }

    s.present |= COLON_PRESENT;
    parser_advance (parser->base);
    WRAP (parse_mus_stop (parser, &s, e));
    return musb_accept_key (&parser->builder, s, e);
  }

  // No leading number — must be ':'
  if (parser_match (parser->base, TT_COLON))
  {
    s.present |= COLON_PRESENT;
    parser_advance (parser->base);
    WRAP (parse_mus_stop (parser, &s, e));
    return musb_accept_key (&parser->builder, s, e);
  }

  return error_causef (
      e,
      ERR_SYNTAX,
      "Expected number or ':' at position %u",
      parser->base->pos
  );
}

static err_t
parse_multi_user_stride_inner (
    struct multi_user_stride_parser *parser,
    error                           *e
)
{
  // Check for empty: []
  if (parser_match (parser->base, TT_RIGHT_BRACKET))
  {
    return SUCCESS;
  }

  WRAP (parse_entry (parser, e));

  while (parser_match (parser->base, TT_COMMA))
  {
    parser_advance (parser->base);
    WRAP (parse_entry (parser, e));
  }

  return SUCCESS;
}

err_t
parse_multi_user_stride (
    struct parser            *parser,
    struct multi_user_stride *dest,
    struct chunk_alloc       *alloc,
    error                    *e
)
{
  struct multi_user_stride_parser p = {
      .base       = parser,
      .persistent = alloc,
  };

  chunk_alloc_create_default (&p.temp);
  musb_create (&p.builder, &p.temp, alloc);

  if (unlikely ((parser_expect (p.base, TT_LEFT_BRACKET, e)) < SUCCESS))
  {
    goto theend;
  }
  if (unlikely ((parse_multi_user_stride_inner (&p, e)) < SUCCESS))
  {
    goto theend;
  }
  if (unlikely ((parser_expect (p.base, TT_RIGHT_BRACKET, e)) < SUCCESS))
  {
    goto theend;
  }
  if (unlikely ((musb_build (dest, &p.builder, e)) < SUCCESS))
  {
    goto theend;
  }

theend:
  chunk_alloc_free_all (&p.temp);
  return error_trace (e);
}

err_t
compile_multi_user_stride (
    struct multi_user_stride *dest,
    const char               *text,
    struct chunk_alloc       *dalloc,
    error                    *e
)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_multi_user_stride (&parser, dest, dalloc, e);
  lex_free (&lex);
  return ret;
}

#ifndef NTEST
TEST (compile_multi_user_stride)
{
  error                    e      = error_create ();
  struct multi_user_stride stride = {0};
  struct chunk_alloc       alloc;
  chunk_alloc_create_default (&alloc);

  // -------------------------------------------------------------------------
  // Empty array
  // -------------------------------------------------------------------------
  TEST_CASE ("[ ]")
  {
    compile_multi_user_stride (&stride, "[]", &alloc, &e);
    test_assert_int_equal (stride.len, 0);
    test_assert_equal (stride.strides, NULL);
    compile_multi_user_stride (&stride, "[ ]", &alloc, &e);
    test_assert_int_equal (stride.len, 0);
    test_assert_equal (stride.strides, NULL);
    compile_multi_user_stride (&stride, " [ ]", &alloc, &e);
    test_assert_int_equal (stride.len, 0);
    test_assert_equal (stride.strides, NULL);
    compile_multi_user_stride (&stride, " [] ", &alloc, &e);
    test_assert_int_equal (stride.len, 0);
    test_assert_equal (stride.strides, NULL);
  }

  // -------------------------------------------------------------------------
  // Single bare index
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0 ]")
  {
    compile_multi_user_stride (&stride, "[0]", &alloc, &e);
    test_assert_int_equal (stride.len, 1);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, START_PRESENT);
    test_assert_int_equal (stride.strides[0].start, 0);
  }

  // -------------------------------------------------------------------------
  // Two bare indices
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    // first
    test_assert_int_equal (stride.strides[0].present, START_PRESENT);
    test_assert_int_equal (stride.strides[0].start, 0);
    // second
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // start + colon, no stop, no step  →  "0:"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0: ]")
  {
    compile_multi_user_stride (&stride, "[0:]", &alloc, &e);
    test_assert_int_equal (stride.len, 1);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        START_PRESENT | COLON_PRESENT
    );
    test_assert_int_equal (stride.strides[0].start, 0);
  }

  // -------------------------------------------------------------------------
  // "0:" followed by a bare index
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0:, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0:, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        START_PRESENT | COLON_PRESENT
    );
    test_assert_int_equal (stride.strides[0].start, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Colon only  →  ":"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ :, 0 ]")
  {
    compile_multi_user_stride (&stride, "[:, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, COLON_PRESENT);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Colon + stop  →  ":0"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ :0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[:0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        COLON_PRESENT | STOP_PRESENT
    );
    test_assert_int_equal (stride.strides[0].stop, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Double colon, no numbers  →  "::"
  // parse_entry sets COLON_PRESENT, parse_step also matches ':' and sets it
  // again (no-op), no integers → only COLON_PRESENT in present
  // -------------------------------------------------------------------------
  TEST_CASE ("[ ::, 0 ]")
  {
    compile_multi_user_stride (&stride, "[::, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (stride.strides[0].present, COLON_PRESENT);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Double colon + step  →  "::0"
  // COLON_PRESENT (from first ':') | STEP_PRESENT (from "::0")
  // Note: no START_PRESENT, no STOP_PRESENT
  // -------------------------------------------------------------------------
  TEST_CASE ("[ ::0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[::0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        COLON_PRESENT | STEP_PRESENT
    );
    test_assert_int_equal (stride.strides[0].step, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Colon + stop + colon, no step  →  ":0:"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ :0:, 0 ]")
  {
    compile_multi_user_stride (&stride, "[:0:, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        COLON_PRESENT | STOP_PRESENT
    );
    test_assert_int_equal (stride.strides[0].stop, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Colon + stop + colon + step  →  ":0:0"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ :0:0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[:0:0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        COLON_PRESENT | STOP_PRESENT | STEP_PRESENT
    );
    test_assert_int_equal (stride.strides[0].stop, 0);
    test_assert_int_equal (stride.strides[0].step, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Start + double colon, no stop/step  →  "0::"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0::, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0::, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        START_PRESENT | COLON_PRESENT
    );
    test_assert_int_equal (stride.strides[0].start, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Start + double colon + step  →  "0::0"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0::0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0::0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        START_PRESENT | COLON_PRESENT | STEP_PRESENT
    );
    test_assert_int_equal (stride.strides[0].start, 0);
    test_assert_int_equal (stride.strides[0].step, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // -------------------------------------------------------------------------
  // Full slice  →  "0:0:0"
  // -------------------------------------------------------------------------
  TEST_CASE ("[ 0:0:0, 0 ]")
  {
    compile_multi_user_stride (&stride, "[0:0:0, 0]", &alloc, &e);
    test_assert_int_equal (stride.len, 2);
    test_assert (stride.strides != NULL);
    test_assert_int_equal (
        stride.strides[0].present,
        START_PRESENT | COLON_PRESENT | STOP_PRESENT | STEP_PRESENT
    );
    test_assert_int_equal (stride.strides[0].start, 0);
    test_assert_int_equal (stride.strides[0].stop, 0);
    test_assert_int_equal (stride.strides[0].step, 0);
    test_assert_int_equal (stride.strides[1].present, START_PRESENT);
    test_assert_int_equal (stride.strides[1].start, 0);
  }

  // =========================================================================
  // Error conditions
  // =========================================================================

  // Missing opening bracket
  TEST_CASE ("error: no leading '['")
  {
    err_t err = compile_multi_user_stride (&stride, "0, 1]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Missing closing bracket
  TEST_CASE ("error: no trailing ']'")
  {
    err_t err = compile_multi_user_stride (&stride, "[0, 1", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Empty input
  TEST_CASE ("error: empty string")
  {
    err_t err = compile_multi_user_stride (&stride, "", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Trailing comma with no entry after it  →  parse_entry gets ']', not a
  // number or ':', so it returns ERR_SYNTAX
  TEST_CASE ("error: trailing comma '[0,]'")
  {
    err_t err = compile_multi_user_stride (&stride, "[0,]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Leading comma — parse_entry gets ',' which is neither number nor ':'
  TEST_CASE ("error: leading comma '[,0]'")
  {
    err_t err = compile_multi_user_stride (&stride, "[,0]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Bare comma between two commas
  TEST_CASE ("error: double comma '[0,,1]'")
  {
    err_t err = compile_multi_user_stride (&stride, "[0,,1]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Garbage token (not number, colon, comma, or bracket)
  TEST_CASE ("error: garbage token '[abc]'")
  {
    err_t err = compile_multi_user_stride (&stride, "[abc]", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  // Completely wrong structure
  TEST_CASE ("error: only a number, no brackets")
  {
    err_t err = compile_multi_user_stride (&stride, "42", &alloc, &e);
    test_assert (err < SUCCESS);
    test_assert_int_equal (e.cause_code, ERR_SYNTAX);
    e.cause_code = 0;
    e.cmlen      = 0;
  }

  chunk_alloc_free_all (&alloc);
}
#endif

/******************************************************************************
 * SECTION: Query
 * ----------------------------------------------------------------------------
 * query   ::= command ';'
 * command ::= read | create | delete | get | exit | help
 * read    ::= 'read'   IDENT user_stride?
 * create  ::= 'create' IDENT type
 * delete  ::= 'delete' IDENT
 * get     ::= 'get'    IDENT
 * exit    ::= 'exit'
 * help    ::= 'help' ( 'read' | 'create' | 'delete' | 'get' | 'exit' | 'help')?
 ******************************************************************************/

static err_t
parse_query_help (struct parser *parser, struct query *dest, error *e)
{
  // HELP
  WRAP (parser_expect (parser, TT_HELP, e));
  dest->type = QT_HELP;

  if (parser_match (parser, TT_READ))
  {
    dest->help.command     = QT_READ;
    dest->help.has_command = true;
    parser_advance (parser);
  }

  else if (parser_match (parser, TT_CREATE))
  {
    dest->help.command     = QT_CREATE;
    dest->help.has_command = true;
    parser_advance (parser);
  }

  else if (parser_match (parser, TT_DELETE))
  {
    dest->help.command     = QT_DELETE;
    dest->help.has_command = true;
    parser_advance (parser);
  }

  else if (parser_match (parser, TT_GET))
  {
    dest->help.command     = QT_GET;
    dest->help.has_command = true;
    parser_advance (parser);
  }

  else if (parser_match (parser, TT_EXIT))
  {
    dest->help.command     = QT_EXIT;
    dest->help.has_command = true;
    parser_advance (parser);
  }

  else if (parser_match (parser, TT_HELP))
  {
    dest->help.command     = QT_HELP;
    dest->help.has_command = true;
    parser_advance (parser);
  }
  else
  {
    dest->help.has_command = false;
  }

  return SUCCESS;
}

static err_t
parse_query_exit (struct parser *parser, struct query *dest, error *e)
{
  // exit
  WRAP (parser_expect (parser, TT_EXIT, e));

  *dest = (struct query){
      .type = QT_EXIT,
  };

  return SUCCESS;
}

static err_t
parse_query_get (struct parser *parser, struct query *dest, error *e)
{
  // GET
  WRAP (parser_expect (parser, TT_GET, e));

  // IDENT
  if (!parser_match (parser, TT_IDENTIFIER))
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected identifier at position %u",
        parser->pos
    );
  }
  struct token *tok = parser_advance (parser);

  *dest = (struct query){
      .type   = QT_GET,
      .create = {
          .vname = (struct string){
              .data = (char *)tok->str.data,
              .len  = tok->str.len,
          },
      },
  };

  return SUCCESS;
}

static err_t
parse_query_delete (struct parser *parser, struct query *dest, error *e)
{
  // DELETE
  WRAP (parser_expect (parser, TT_DELETE, e));

  // IDENT
  if (!parser_match (parser, TT_IDENTIFIER))
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected identifier at position %u",
        parser->pos
    );
  }
  struct token *tok = parser_advance (parser);

  *dest = (struct query){
      .type   = QT_DELETE,
      .create = {
          .vname = (struct string){
              .data = (char *)tok->str.data,
              .len  = tok->str.len,
          },
      },
  };

  return SUCCESS;
}

static err_t
parse_query_create (
    struct parser      *parser,
    struct query       *dest,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  // CREATE
  WRAP (parser_expect (parser, TT_CREATE, e));

  // IDENT
  if (!parser_match (parser, TT_IDENTIFIER))
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected identifier at position %u",
        parser->pos
    );
  }
  struct token *tok = parser_advance (parser);

  // TYPE
  struct type *t = chunk_malloc (dalloc, 1, sizeof *t, e);
  if (t == NULL)
  {
    return error_trace (e);
  }
  WRAP (parse_type (parser, t, dalloc, e));

  *dest = (struct query){
      .type   = QT_CREATE,
      .create = {
          .vname =
              (struct string){
                  .data = (char *)tok->str.data,
                  .len  = tok->str.len,
              },
          .type = t,
      },
  };

  return SUCCESS;
}

static err_t
parse_query_read (struct parser *parser, struct query *dest, error *e)
{
  // READ
  WRAP (parser_expect (parser, TT_READ, e));

  // IDENT
  if (!parser_match (parser, TT_IDENTIFIER))
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected identifier at position %u",
        parser->pos
    );
  }
  struct token *tok = parser_advance (parser);

  // USER_STRIDE
  struct user_stride ustr = ustride ();

  if (parser_match (parser, TT_LEFT_BRACKET))
  {
    WRAP (parse_user_stride (parser, &ustr, e));
  }

  *dest = (struct query){
      .type = QT_READ,
      .read = {
          .vname =
              (struct string){
                  .data = (char *)tok->str.data,
                  .len  = tok->str.len,
              },
          .ustr = ustr,
      },
  };

  return SUCCESS;
}

err_t
parse_query (
    struct parser      *parser,
    struct query       *dest,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  if (parser_match (parser, TT_READ))
  {
    WRAP (parse_query_read (parser, dest, e));
  }

  else if (parser_match (parser, TT_CREATE))
  {
    WRAP (parse_query_create (parser, dest, dalloc, e));
  }

  else if (parser_match (parser, TT_DELETE))
  {
    WRAP (parse_query_delete (parser, dest, e));
  }

  else if (parser_match (parser, TT_GET))
  {
    WRAP (parse_query_get (parser, dest, e));
  }

  else if (parser_match (parser, TT_EXIT))
  {
    WRAP (parse_query_exit (parser, dest, e));
  }

  else if (parser_match (parser, TT_HELP))
  {
    WRAP (parse_query_help (parser, dest, e));
  }
  else
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected a valid operation at pos: %u",
        parser->pos
    );
  }

  return parser_expect (parser, TT_SEMICOLON, e);
}

err_t
compile_query (
    struct query       *dest,
    const char         *text,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_query (&parser, dest, dalloc, e);
  lex_free (&lex);
  return ret;
}

#ifndef NTEST

static void
test_query_green_path (const char *query, struct query expected)
{
  struct chunk_alloc alloc;
  chunk_alloc_create_default (&alloc);
  struct query actual;
  error        e = error_create ();

  TEST_CASE ("SHOULD PASS: %s", query)
  {
    compile_query (&actual, query, &alloc, &e);
    test_assert (query_equal (&actual, &expected));
  }

  chunk_alloc_free_all (&alloc);
}

static void
test_query_red_path (const char *query, err_t code)
{
  struct chunk_alloc alloc;
  chunk_alloc_create_default (&alloc);
  struct query actual;
  error        e = error_create ();

  TEST_CASE ("SHOULD FAIL: %s", query)
  {
    test_err_t_check (compile_query (&actual, query, &alloc, &e), code, &e);
  }

  chunk_alloc_free_all (&alloc);
}

TEST (compile_query)
{
  // READ
  {
    test_query_red_path ("read", ERR_SYNTAX);
    test_query_red_path ("read;", ERR_SYNTAX);
    test_query_green_path (
        "read foo;",
        (struct query){
            .type = QT_READ,
            .read = {.vname = strfcstr ("foo"), .ustr = ustride ()}
        }
    );
    test_query_green_path (
        "read foo[0:10:20];",
        (struct query){
            .type = QT_READ,
            .read = {.vname = strfcstr ("foo"), .ustr = ustride012 (0, 10, 20)},
        }
    );
  }

  // CREATE
  {
    test_query_red_path ("create", ERR_SYNTAX);
    test_query_red_path ("create 1;", ERR_SYNTAX);
    test_query_red_path ("create foo;", ERR_SYNTAX);
    test_query_red_path ("create foo 1;", ERR_SYNTAX);
    test_query_red_path ("create a i32", ERR_SYNTAX);

    // Sarray
    u32         dims[2] = {10, 20};
    struct type t0      = {
        .type = T_SARRAY,
        .sa   = {.rank = 2, .dims = dims, .t = &TF32}
    };

    // Union
    struct type  *utypes[2] = {&TI32, &t0};
    struct string ukeys[2]  = {strfcstr ("c"), strfcstr ("d")};
    struct type   t1        = {
        .type = T_UNION,
        .un   = {
            .len   = 2,
            .types = utypes,
            .keys  = ukeys,
        },
    };

    // Struct
    struct type  *stypes[2] = {&TI32, &t1};
    struct string skeys[2]  = {strfcstr ("a"), strfcstr ("b")};
    struct type   t2        = {
        .type = T_STRUCT,
        .st   = {
            .len   = 2,
            .types = stypes,
            .keys  = skeys,
        },
    };
    test_query_green_path (
        "create foo struct { a i32, b union { c i32, d [10][20]f32 } };",
        (struct query){
            .type   = QT_CREATE,
            .create = {
                .vname = strfcstr ("foo"),
                .type  = &t2,
            },
        }
    );
  }

  // DELETE
  {
    test_query_red_path ("delete", ERR_SYNTAX);
    test_query_red_path ("delete 1;", ERR_SYNTAX);
    test_query_red_path ("delete foo", ERR_SYNTAX);
    test_query_green_path (
        "delete foo;",
        (struct query){
            .type   = QT_DELETE,
            .delete = {
                .vname = strfcstr ("foo"),
            },
        }
    );
  }

  // GET
  {
    test_query_red_path ("get", ERR_SYNTAX);
    test_query_red_path ("get 1;", ERR_SYNTAX);
    test_query_red_path ("get foo", ERR_SYNTAX);
    test_query_green_path (
        "get foo;",
        (struct query){
            .type = QT_GET,
            .get  = {
                .vname = strfcstr ("foo"),
            },
        }
    );
  }

  {
    test_query_red_path ("exit", ERR_SYNTAX);
    test_query_red_path ("exit 1;", ERR_SYNTAX);
    test_query_red_path ("exit foo", ERR_SYNTAX);
    test_query_green_path (
        "exit;",
        (struct query){
            .type = QT_EXIT,
        }
    );
  }

  {
    test_query_red_path ("help", ERR_SYNTAX);
    test_query_red_path ("help 1;", ERR_SYNTAX);
    test_query_red_path ("help foo", ERR_SYNTAX);
    test_query_red_path ("help read", ERR_SYNTAX);
    test_query_red_path ("help foo;", ERR_SYNTAX);
    test_query_green_path (
        "help;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = false},
        }
    );
    test_query_green_path (
        "help read;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_READ},
        }
    );
    test_query_green_path (
        "help create;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_CREATE},
        }
    );
    test_query_green_path (
        "help delete;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_DELETE},
        }
    );
    test_query_green_path (
        "help get;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_GET},
        }
    );
    test_query_green_path (
        "help exit;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_EXIT},
        }
    );
    test_query_green_path (
        "help help;",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_HELP},
        }
    );
  }
}
#endif

/******************************************************************************
 * SECTION: Sub Type
 * ----------------------------------------------------------------------------
 * subtype ::= IDENT ( multi_user_stride | '.' IDENT )*
 ******************************************************************************/

struct sub_type_parser
{
  struct parser      *base;
  struct subtype     *dest;
  struct chunk_alloc  temp;
  struct chunk_alloc *persistent;
};

static err_t
parse_sub_type_inner (struct sub_type_parser *parser, error *e)
{
  if (!parser_match (parser->base, TT_IDENTIFIER))
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected variable name at position %u",
        parser->base->pos
    );
  }

  // VNAME
  struct token *tok   = parser_advance (parser->base);
  struct string vname = {.data = tok->str.data, .len = tok->str.len};

  // Type accessors
  struct type_accessor_builder tab;
  tab_create (&tab, &parser->temp, parser->persistent);
  while (true)
  {
    // Stride
    if (parser_match (parser->base, TT_LEFT_BRACKET))
    {
      struct multi_user_stride stride;
      WRAP (
          parse_multi_user_stride (parser->base, &stride, parser->persistent, e)
      );
      for (u32 i = 0; i < stride.len; ++i)
      {
        WRAP (tab_accept_stride (&tab, stride.strides[i], e));
      }
    }

    // Dot
    else if (parser_match (parser->base, TT_DOT))
    {
      parser_advance (parser->base);
      if (!parser_match (parser->base, TT_IDENTIFIER))
      {
        return error_causef (
            e,
            ERR_SYNTAX,
            "Expected "
            "identifier at "
            "position %u",
            parser->base->pos
        );
      }

      tok                  = parser_advance (parser->base);
      struct string select = {
          .data = tok->str.data,
          .len  = tok->str.len,
      };
      WRAP (tab_accept_select (&tab, select, e));
    }

    // Done
    else
    {
      break;
    }
  }

  struct type_accessor ta;
  WRAP (tab_build (&ta, &tab, e));

  return subtype_create (parser->dest, vname, ta, e);
}

err_t
parse_subtype (
    struct parser      *p,
    struct subtype     *dest,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  struct sub_type_parser parser = {
      .base       = p,
      .dest       = dest,
      .persistent = dalloc,
  };

  chunk_alloc_create_default (&parser.temp);

  err_t rc = parse_sub_type_inner (&parser, e);

  chunk_alloc_free_all (&parser.temp);

  return rc;
}

err_t
compile_subtype (
    struct subtype     *dest,
    const char         *text,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_subtype (&parser, dest, dalloc, e);
  lex_free (&lex);
  return ret;
}

// TODO - Sub type tests

/******************************************************************************
 * SECTION: Type
 * ----------------------------------------------------------------------------
 * type           ::= struct_type | union_type | sarray_type | primitive_type
 * primitive_type ::= PRIM
 * sarray_type    ::= ( '[' INTEGER ']' )+ type
 * struct_type    ::= 'struct' '{' field ( ',' field )* '}'
 * union_type     ::= 'union'  '{' field ( ',' field )* '}'
 * field          ::= IDENT type
 ******************************************************************************/

struct type_parser
{
  struct parser      *base;
  struct type        *dest;
  struct chunk_alloc  temp;
  struct chunk_alloc *persistent;
};

static err_t
parse_type_inner (struct type_parser *parser, struct type *out, error *e);

static err_t
parse_primitive_type (struct type_parser *parser, struct type *out, error *e)
{
  if (!parser_match (parser->base, TT_PRIM))
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected primitive type at position %u",
        parser->base->pos
    );
  }

  struct token *tok = parser_advance (parser->base);
  out->type         = T_PRIM;
  out->p            = tok->prim;

  return SUCCESS;
}

static err_t
parse_sarray_type (struct type_parser *parser, struct type *out, error *e)
{
  struct sarray_builder builder;
  sab_create (&builder, &parser->temp, parser->persistent);

  // Parse all [N] brackets
  while (parser_match (parser->base, TT_LEFT_BRACKET))
  {
    WRAP (parser_expect (parser->base, TT_LEFT_BRACKET, e));

    if (!parser_match (parser->base, TT_INTEGER))
    {
      return error_causef (
          e,
          ERR_SYNTAX,
          "Expected array size at position "
          "%u",
          parser->base->pos
      );
    }

    struct token *tok = parser_advance (parser->base);

    WRAP (sab_accept_dim (&builder, tok->integer, e));
    WRAP (parser_expect (parser->base, TT_RIGHT_BRACKET, e));
  }

  // Inner most type
  struct type *inner = chunk_malloc (parser->persistent, 1, sizeof *inner, e);
  if (inner == NULL)
  {
    return error_trace (e);
  }
  WRAP (parse_type_inner (parser, inner, e));
  WRAP (sab_accept_type (&builder, inner, e));

  out->type = T_SARRAY;
  return sab_build (&out->sa, &builder, e);
}

static err_t
parse_field (
    struct kvt_list_builder *builder,
    struct type_parser      *parser,
    error                   *e
)
{
  // IDENT
  if (!parser_match (parser->base, TT_IDENTIFIER))
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected identifier at position %u",
        parser->base->pos
    );
  }

  struct token *tok = parser_advance (parser->base);
  WRAP (kvlb_accept_key (
      builder,
      (struct string){
          .data = (char *)tok->str.data,
          .len  = tok->str.len,
      },
      e
  ));

  // Type
  struct type *inner = chunk_malloc (parser->persistent, 1, sizeof *inner, e);
  if (inner == NULL)
  {
    return error_trace (e);
  }
  WRAP (parse_type_inner (parser, inner, e));
  WRAP (kvlb_accept_type (builder, inner, e));

  return SUCCESS;
}

static err_t
parse_struct_type (struct type_parser *parser, struct type *out, error *e)
{
  // 'struct'
  WRAP (parser_expect (parser->base, TT_STRUCT, e));

  // '{ '
  WRAP (parser_expect (parser->base, TT_LEFT_BRACE, e));

  struct kvt_list_builder builder;
  kvlb_create (&builder, &parser->temp, parser->persistent);

  WRAP (parse_field (&builder, parser, e));

  while (parser_match (parser->base, TT_COMMA))
  {
    parser_advance (parser->base);
    WRAP (parse_field (&builder, parser, e));
  }

  WRAP (parser_expect (parser->base, TT_RIGHT_BRACE, e));

  // Build kvt list
  struct kvt_list list;
  WRAP (kvlb_build (&list, &builder, e));

  out->type = T_STRUCT;

  return struct_t_create (&out->st, list, NULL, e);
}

static err_t
parse_union_type (struct type_parser *parser, struct type *out, error *e)
{
  // 'union'
  WRAP (parser_expect (parser->base, TT_UNION, e));

  // '{ '
  WRAP (parser_expect (parser->base, TT_LEFT_BRACE, e));

  struct kvt_list_builder builder;
  kvlb_create (&builder, &parser->temp, parser->persistent);

  WRAP (parse_field (&builder, parser, e));

  while (parser_match (parser->base, TT_COMMA))
  {
    parser_advance (parser->base);
    WRAP (parse_field (&builder, parser, e));
  }

  WRAP (parser_expect (parser->base, TT_RIGHT_BRACE, e));

  // Build kvt list
  struct kvt_list list;
  WRAP (kvlb_build (&list, &builder, e));

  out->type = T_UNION;

  return union_t_create (&out->un, list, NULL, e);
}

static err_t
parse_type_inner (struct type_parser *parser, struct type *out, error *e)
{
  struct token *tok = parser_peek (parser->base);

  switch (tok->type)
  {
    case TT_STRUCT:
    {
      return parse_struct_type (parser, out, e);
    }
    case TT_UNION:
    {
      return parse_union_type (parser, out, e);
    }
    case TT_LEFT_BRACKET:
    {
      return parse_sarray_type (parser, out, e);
    }
    case TT_PRIM:
    {
      return parse_primitive_type (parser, out, e);
    }
    default:
    {
      return error_causef (
          e,
          ERR_SYNTAX,
          "Expected type at position %u, got token "
          "type %s",
          parser->base->pos,
          tt_tostr (tok->type)
      );
    }
  }
}

err_t
parse_type (
    struct parser      *p,
    struct type        *dest,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  struct type_parser parser = {
      .base       = p,
      .dest       = dest,
      .persistent = dalloc,
  };

  chunk_alloc_create_default (&parser.temp);

  if (unlikely ((parse_type_inner (&parser, parser.dest, e)) < SUCCESS))
  {
    goto theend;
  }

theend:
  chunk_alloc_free_all (&parser.temp);
  return error_trace (e);
}

err_t
compile_type (
    struct type        *dest,
    const char         *text,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_type (&parser, dest, dalloc, e);
  lex_free (&lex);
  return ret;
}

#ifndef NTEST

static void
test_compile_type_green_path (const char *query, struct type expected)
{
  struct chunk_alloc alloc;
  struct type        actual;
  chunk_alloc_create_default (&alloc);
  error e = error_create ();

  TEST_CASE ("SHOULD PASS: %s", query)
  {
    compile_type (&actual, query, &alloc, &e);
    test_assert (type_equal (&expected, &actual));
  }

  chunk_alloc_free_all (&alloc);
}

static void
test_compile_type_red_path (const char *query, err_t code)
{
  struct chunk_alloc alloc;
  struct type        actual;
  chunk_alloc_create_default (&alloc);
  error e = error_create ();

  TEST_CASE ("SHOULD FAIL: %s", query)
  {
    test_err_t_check (compile_type (&actual, query, &alloc, &e), code, &e);
  }

  chunk_alloc_free_all (&alloc);
}

TEST (compile_type)
{
  test_compile_type_green_path ("i8", TI8);
  test_compile_type_green_path ("i16", TI16);
  test_compile_type_green_path ("i32", TI32);
  test_compile_type_green_path ("i64", TI64);

  test_compile_type_green_path ("u8", TU8);
  test_compile_type_green_path ("u16", TU16);
  test_compile_type_green_path ("u32", TU32);
  test_compile_type_green_path ("u64", TU64);

  test_compile_type_green_path ("f16", TF16);
  test_compile_type_green_path ("f32", TF32);
  test_compile_type_green_path ("f64", TF64);
  test_compile_type_green_path ("f128", TF128);

  test_compile_type_green_path ("cf32", TCF32);
  test_compile_type_green_path ("cf64", TCF64);
  test_compile_type_green_path ("cf128", TCF128);
  test_compile_type_green_path ("cf256", TCF256);

  test_compile_type_red_path ("i2", ERR_SYNTAX);

  // SARRAY
  test_compile_type_green_path ("[10]i32", mk_sarray (1, (u32[]){10}, &TI32));
  test_compile_type_green_path (
      "[5][10]f64",
      mk_sarray (2, (u32[]){5, 10}, &TF64)
  );
  test_compile_type_green_path (
      "[2][3][4]u8",
      mk_sarray (3, (u32[]){2, 3, 4}, &TU8)
  );

  // STRUCT
  test_compile_type_green_path (
      "struct { x i32, y f64 }",
      mk_struct (
          2,
          (struct string[]){
              strfcstr ("x"),
              strfcstr ("y"),
          },
          (struct type *[]){
              &TI32,
              &TF64,
          }
      )
  );

  struct type inner = mk_struct (
      1,
      (struct string[]){strfcstr ("b")},
      (struct type *[]){&TI32}
  );
  test_compile_type_green_path (
      "struct { a struct { b i32 } }",
      mk_struct (
          1,
          (struct string[]){strfcstr ("a")},
          (struct type *[]){&inner}
      )
  );

  // UNION
  test_compile_type_green_path (
      "union { x i32, y f64 }",
      mk_union (
          2,
          (struct string[]){
              strfcstr ("x"),
              strfcstr ("y"),
          },
          (struct type *[]){
              &TI32,
              &TF64,
          }
      )
  );

  // COMPLICATED
  struct type inner_sarray = mk_sarray (1, (u32[]){5}, &TF64);
  struct type inner_struct = mk_struct (
      1,
      (struct string[]){strfcstr ("x"), strfcstr ("y")},
      (struct type *[]){&TI32, &inner_sarray}
  );
  test_compile_type_green_path (
      "[10]struct { x i32, y [5]f64 }",
      mk_sarray (1, (u32[]){10}, &inner_struct)
  );
}

#endif

/******************************************************************************
 * SECTION: Type Ref
 * ----------------------------------------------------------------------------
 * type_ref        ::= struct_type_ref | take_type_ref
 * struct_type_ref ::= 'struct' '{' field_ref ( ',' field_ref )* '}'
 * field_ref       ::= IDENT type_ref
 * take_type_ref   ::= subtype
 ******************************************************************************/

struct type_ref_parser
{
  struct parser      *base;
  struct type_ref    *dest;
  struct chunk_alloc  temp;
  struct chunk_alloc *persistent;
};

static err_t parse_type_ref_inner (
    struct type_ref_parser *parser,
    struct type_ref        *out,
    error                  *e
);

static err_t
parse_take_type_ref (
    struct type_ref_parser *parser,
    struct type_ref        *out,
    error                  *e
)
{
  struct subtype st;
  WRAP (parse_subtype (parser->base, &st, parser->persistent, e));

  out->type     = TR_TAKE;
  out->tk.vname = st.vname;
  out->tk.ta    = st.ta;

  return SUCCESS;
}

static err_t
parse_field_ref (
    struct kvt_ref_list_builder *builder,
    struct type_ref_parser      *parser,
    error                       *e
)
{
  // IDENT
  if (!parser_match (parser->base, TT_IDENTIFIER))
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected identifier at position %u",
        parser->base->pos
    );
  }

  struct token *tok = parser_advance (parser->base);
  WRAP (kvrlb_accept_key (
      builder,
      (struct string){
          .data = (char *)tok->str.data,
          .len  = tok->str.len,
      },
      e
  ));

  // Type ref
  struct type_ref inner;
  WRAP (parse_type_ref_inner (parser, &inner, e));
  WRAP (kvrlb_accept_type (builder, inner, e));

  return SUCCESS;
}

static err_t
parse_struct_type_ref (
    struct type_ref_parser *parser,
    struct type_ref        *out,
    error                  *e
)
{
  // 'struct'
  WRAP (parser_expect (parser->base, TT_STRUCT, e));

  // '{ '
  WRAP (parser_expect (parser->base, TT_LEFT_BRACE, e));

  struct kvt_ref_list_builder builder;
  kvrlb_create (&builder, &parser->temp, parser->persistent);

  WRAP (parse_field_ref (&builder, parser, e));

  while (parser_match (parser->base, TT_COMMA))
  {
    parser_advance (parser->base);
    WRAP (parse_field_ref (&builder, parser, e));
  }

  WRAP (parser_expect (parser->base, TT_RIGHT_BRACE, e));

  // Build kvt_ref list
  struct kvt_ref_list list;
  WRAP (kvrlb_build (&list, &builder, e));

  out->type = TR_STRUCT;
  out->st   = (struct struct_tr){
      .len   = list.len,
      .keys  = list.keys,
      .types = list.types,
  };

  return SUCCESS;
}

static err_t
parse_type_ref_inner (
    struct type_ref_parser *parser,
    struct type_ref        *out,
    error                  *e
)
{
  struct token *tok = parser_peek (parser->base);

  switch (tok->type)
  {
    case TT_STRUCT:
    {
      return parse_struct_type_ref (parser, out, e);
    }
    case TT_IDENTIFIER:
    {
      return parse_take_type_ref (parser, out, e);
    }
    default:
    {
      return error_causef (
          e,
          ERR_SYNTAX,
          "Expected type_ref (struct or identifier) "
          "at "
          "position %u, got token type %s",
          parser->base->pos,
          tt_tostr (tok->type)
      );
    }
  }
}

err_t
parse_type_ref (
    struct parser      *p,
    struct type_ref    *dest,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  struct type_ref_parser parser = {
      .base       = p,
      .dest       = dest,
      .persistent = dalloc,
  };

  chunk_alloc_create_default (&parser.temp);

  if (unlikely ((parse_type_ref_inner (&parser, parser.dest, e)) < SUCCESS))
  {
    goto theend;
  }

theend:
  chunk_alloc_free_all (&parser.temp);
  return error_trace (e);
}

err_t
compile_type_ref (
    struct type_ref    *dest,
    const char         *text,
    struct chunk_alloc *dalloc,
    error              *e
)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_type_ref (&parser, dest, dalloc, e);

  lex_free (&lex);

  return ret;
}

#ifndef NTEST

static void
test_compile_type_ref_green_path (const char *query, struct type_ref expected)
{
  struct chunk_alloc alloc;
  struct type_ref    actual;
  chunk_alloc_create_default (&alloc);
  error e = error_create ();

  TEST_CASE ("SHOULD PASS: %s", query)
  {
    compile_type_ref (&actual, query, &alloc, &e);
    test_assert (type_ref_equal (expected, actual));
  }

  chunk_alloc_free_all (&alloc);
}

static void
test_compile_type_ref_red_path (const char *query, err_t code)
{
  struct chunk_alloc alloc;
  struct type_ref    actual;
  chunk_alloc_create_default (&alloc);
  error e = error_create ();

  TEST_CASE ("SHOULD FAIL: %s", query)
  {
    test_err_t_check (compile_type_ref (&actual, query, &alloc, &e), code, &e);
  }

  chunk_alloc_free_all (&alloc);
}

TEST (compile_type_ref)
{
  test_compile_type_ref_green_path (
      "myvar",
      tr_take (strfcstr ("myvar"), ta_take ())
  );

  test_compile_type_ref_green_path (
      "myvar[9]",
      tr_take (
          strfcstr ("myvar"),
          ta_range ((struct user_stride[]){ustride_single (9)}, 1, &ta_take ())
      )
  );

  test_compile_type_ref_green_path (
      "myvar.field",
      tr_take (strfcstr ("myvar"), ta_select (strfcstr ("field"), &ta_take ()))
  );

  struct type_accessor subrange =
      ta_range ((struct user_stride[]){ustride_single (0)}, 1, &ta_take ());
  test_compile_type_ref_green_path (
      "myvar.a[0]",
      tr_take (strfcstr ("myvar"), ta_select (strfcstr ("a"), &subrange))
  );

  test_compile_type_ref_green_path (
      "struct { a myvar }",
      tr_struct (
          1,
          (struct string[]){strfcstr ("a")},
          (struct type_ref[]){tr_take (strfcstr ("myvar"), ta_take ())}
      )
  );

  test_compile_type_ref_green_path (
      "struct { a x, b y }",
      tr_struct (
          2,
          (struct string[]){strfcstr ("a"), strfcstr ("b")},
          (struct type_ref[]){
              tr_take (strfcstr ("x"), ta_take ()),
              tr_take (strfcstr ("y"), ta_take ()),
          }
      )
  );

  test_compile_type_ref_green_path (
      "struct { a x, b y, c z }",
      tr_struct (
          3,
          (struct string[]){strfcstr ("a"), strfcstr ("b"), strfcstr ("c")},
          (struct type_ref[]){
              tr_take (strfcstr ("x"), ta_take ()),
              tr_take (strfcstr ("y"), ta_take ()),
              tr_take (strfcstr ("z"), ta_take ()),
          }
      )
  );

  test_compile_type_ref_green_path (
      "struct { a struct { b x } }",
      tr_struct (
          1,
          (struct string[]){strfcstr ("a")},
          (struct type_ref[]){
              tr_struct (
                  1,
                  (struct string[]){strfcstr ("b")},
                  (struct type_ref[]){tr_take (strfcstr ("x"), ta_take ())}
              ),
          }
      )
  );

  test_compile_type_ref_green_path (
      "struct { a myvar[0] }",
      tr_struct (
          1,
          (struct string[]){strfcstr ("a")},
          (struct type_ref[]){tr_take (
              strfcstr ("myvar"),
              ta_range (
                  (struct user_stride[]){ustride_single (0)},
                  1,
                  &ta_take ()
              )
          )}
      )
  );

  test_compile_type_ref_red_path ("", ERR_SYNTAX);
  test_compile_type_ref_red_path ("42", ERR_SYNTAX);
  test_compile_type_ref_red_path ("[0]", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct a x }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct {}", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x, }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x,, b y }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x, b }", ERR_SYNTAX);
  test_compile_type_ref_red_path ("struct { a x, struct y }", ERR_SYNTAX);
}

#endif

/******************************************************************************
 * SECTION: User Stride
 * ----------------------------------------------------------------------------
 * user_stride ::= '[' ( INTEGER ( ':' INTEGER? ( ':' INTEGER? )? )?
 *                     | ':' INTEGER? ( ':' INTEGER? )?
 *                     ) ']'
 ******************************************************************************/

// Parse optional ':' NUMBER (step)
static err_t
parse_us_step (struct parser *base, struct user_stride *s, error *e)
{
  if (!parser_match (base, TT_COLON))
  {
    return SUCCESS;
  }

  s->present |= COLON_PRESENT;
  parser_advance (base);

  if (parser_match (base, TT_INTEGER))
  {
    struct token *tok = parser_advance (base);
    s->step           = (sb_size)tok->integer;
    s->present |= STEP_PRESENT;
  }

  return SUCCESS;
}

// Parse optional NUMBER (stop), then optional ':' NUMBER (step)
static err_t
parse_us_stop (struct parser *base, struct user_stride *s, error *e)
{
  if (parser_match (base, TT_INTEGER))
  {
    struct token *tok = parser_advance (base);
    s->stop           = (sb_size)tok->integer;
    s->present |= STOP_PRESENT;
  }

  return parse_us_step (base, s, e);
}

err_t
parse_user_stride (struct parser *parser, struct user_stride *dest, error *e)
{
  struct user_stride s = {0};

  WRAP (parser_expect (parser, TT_LEFT_BRACKET, e));

  if (parser_match (parser, TT_INTEGER))
  {
    // Leading integer: start
    struct token *tok = parser_advance (parser);
    s.start           = (sb_size)tok->integer;
    s.present |= START_PRESENT;

    if (parser_match (parser, TT_COLON))
    {
      // start ':' ...
      s.present |= COLON_PRESENT;
      parser_advance (parser);
      WRAP (parse_us_stop (parser, &s, e));
    }
    // else: bare integer — single index, nothing more to parse
  }
  else if (parser_match (parser, TT_COLON))
  {
    // No leading integer: ':' ...
    s.present |= COLON_PRESENT;
    parser_advance (parser);
    WRAP (parse_us_stop (parser, &s, e));
  }
  else
  {
    return error_causef (
        e,
        ERR_SYNTAX,
        "Expected number or ':' at position %u",
        parser->pos
    );
  }

  *dest = s;
  return parser_expect (parser, TT_RIGHT_BRACKET, e);
}

err_t
compile_user_stride (struct user_stride *dest, const char *text, error *e)
{
  struct lexer lex;
  WRAP (lex_tokens (text, strlen (text), &lex, e));

  struct parser parser = parser_init (lex.tokens, lex.ntokens);

  err_t ret = parse_user_stride (&parser, dest, e);
  lex_free (&lex);
  return ret;
}

#ifndef NTEST
static void
test_compile_user_stride_green_path (
    const char        *query,
    struct user_stride expected
)
{
  struct user_stride actual;
  error              e = error_create ();

  TEST_CASE ("SHOULD PASS: %s", query)
  {
    compile_user_stride (&actual, query, &e);
    test_assert (user_stride_equal (&actual, &expected));
  }
}

static void
test_compile_user_stride_red_path (const char *query, err_t code)
{
  struct user_stride actual;
  error              e = error_create ();

  TEST_CASE ("SHOULD FAIL: %s", query)
  {
    test_err_t_check (compile_user_stride (&actual, query, &e), code, &e);
  }
}

TEST (compile_user_stride)
{
  test_compile_user_stride_red_path ("[]", ERR_SYNTAX);
  test_compile_user_stride_red_path ("[ ]", ERR_SYNTAX);
  test_compile_user_stride_red_path (" [ ]", ERR_SYNTAX);

  test_compile_user_stride_green_path ("[5]", ustride_single (5));

  test_compile_user_stride_green_path ("[:]", ustride ());
  test_compile_user_stride_green_path ("[:6]", ustride1 (6));
  test_compile_user_stride_green_path ("[5:]", ustride0 (5));
  test_compile_user_stride_green_path ("[5:6]", ustride01 (5, 6));

  test_compile_user_stride_green_path ("[::]", ustride ());
  test_compile_user_stride_green_path ("[::7]", ustride2 (7));
  test_compile_user_stride_green_path ("[:6:]", ustride1 (6));
  test_compile_user_stride_green_path ("[:6:7]", ustride12 (6, 7));

  test_compile_user_stride_green_path ("[5::]", ustride0 (5));
  test_compile_user_stride_green_path ("[5::7]", ustride02 (5, 7));
  test_compile_user_stride_green_path ("[5:6:]", ustride01 (5, 6));
  test_compile_user_stride_green_path ("[5:6:7]", ustride012 (5, 6, 7));
}
#endif
