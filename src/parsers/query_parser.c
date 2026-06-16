#include "alloc.h"
#include "collections.h"
#include "compiler.h"
#include "error.h"
#include "query.h"

// query         ::= READ | CREATE | DELETE | GET | EXIT | HELP ';'

// help ::= help [COMMAND]
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

// exit ::= exit
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

// get ::= get IDENT
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

// delete ::= delete IDENT
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

// create ::= create <IDENT> <TYPE>
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

// read ::= read IDENT USER_STRIDE
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

// query         ::= READ | CREATE | DELETE | GET | EXIT | HELP ';'
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
