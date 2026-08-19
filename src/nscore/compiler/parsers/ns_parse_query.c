/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.

#include "core/ns_alloc.h"
#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_stdtypes.h"
#include "core/ns_stride.h"
#include "core/ns_string.h"
#include "core/testing/ns_testing.h"
#include "nscore/compiler/ns_compiler.h"
#include "nscore/compiler/ns_lexer.h"
#include "nscore/compiler/ns_tokens.h"
#include "nscore/compiler/parsers/ns_parser.h"
#include "nscore/types/ns_query.h"
#include "nscore/types/ns_types.h"

#include <stdbool.h>
#include <string.h>

/******************************************************************************
 * SECTION: Query
 * ----------------------------------------------------------------------------
 * query   ::= read | create | delete | get | exit | help
 * help    ::= 'help' ( 'read' | 'create' | 'delete' | 'get' | 'exit' | 'help')?
 * exit    ::= 'exit'
 * get     ::= 'get'    IDENT
 * delete  ::= 'delete' IDENT
 * create  ::= 'create' IDENT type
 * insert  ::= 'insert' IDENT ofst len
 * read    ::= 'read'   IDENT user_stride?
 * remove  ::= 'remove' IDENT user_stride?
 * write   ::= 'write'  IDENT user_stride?
 ******************************************************************************/

static err_t
parse_query_help (struct parser *parser, struct query *dest, error *e)
{
  WRAP (parser_expect (parser, TT_HELP, e));
  dest->type = QT_HELP;

  if (parser_match (parser, TT_READ)) {
    dest->help.command     = QT_READ;
    dest->help.has_command = true;
    parser_advance (parser);
  }

  else if (parser_match (parser, TT_CREATE)) {
    dest->help.command     = QT_CREATE;
    dest->help.has_command = true;
    parser_advance (parser);
  }

  else if (parser_match (parser, TT_DELETE)) {
    dest->help.command     = QT_DELETE;
    dest->help.has_command = true;
    parser_advance (parser);
  }

  else if (parser_match (parser, TT_GET)) {
    dest->help.command     = QT_GET;
    dest->help.has_command = true;
    parser_advance (parser);
  }

  else if (parser_match (parser, TT_EXIT)) {
    dest->help.command     = QT_EXIT;
    dest->help.has_command = true;
    parser_advance (parser);
  }

  else if (parser_match (parser, TT_HELP)) {
    dest->help.command     = QT_HELP;
    dest->help.has_command = true;
    parser_advance (parser);
  } else {
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

  // IF EXISTS
  bool if_exists = false;
  if (parser_match (parser, TT_IF)) {
    parser_advance (parser);
    WRAP (parser_expect (parser, TT_EXISTS, e));
    if_exists = true;
  }

  // IDENT
  if (!parser_match (parser, TT_IDENTIFIER)) {
    return error_causef (e, ERR_SYNTAX, "Expected identifier at position %u", parser->pos);
  }
  struct token *tok = parser_advance (parser);

  *dest             = (struct query){
      .type = QT_GET,
      .get  = {
          .name =
              (struct string){
                  .data = (char *)tok->str.data,
                  .len  = tok->str.len,
              },
          .if_exists = if_exists,
      },
  };

  return SUCCESS;
}

static err_t
parse_query_delete (struct parser *parser, struct query *dest, error *e)
{
  // DELETE
  WRAP (parser_expect (parser, TT_DELETE, e));

  // IF EXISTS
  bool if_exists = false;
  if (parser_match (parser, TT_IF)) {
    parser_advance (parser);
    WRAP (parser_expect (parser, TT_EXISTS, e));
    if_exists = true;
  }

  // IDENT
  if (!parser_match (parser, TT_IDENTIFIER)) {
    return error_causef (e, ERR_SYNTAX, "Expected identifier at position %u", parser->pos);
  }
  struct token *tok = parser_advance (parser);

  *dest             = (struct query){
      .type   = QT_DELETE,
      .delete = {
          .name =
              (struct string){
                  .data = (char *)tok->str.data,
                  .len  = tok->str.len,
              },
          .if_exists = if_exists,
      },
  };

  return SUCCESS;
}

static err_t
parse_query_create (struct parser *parser, struct query *dest, error *e)
{
  WRAP (parser_expect (parser, TT_CREATE, e));

  if (!parser_match (parser, TT_IDENTIFIER)) {
    return error_causef (e, ERR_SYNTAX, "Expected identifier at position %u", parser->pos);
  }

  struct token *tok = parser_advance (parser);

  *dest             = (struct query){
      .type   = QT_CREATE,
      .create = {
          .name =
              (struct string){
                  .data = (char *)tok->str.data,
                  .len  = tok->str.len,
              },
          // .type = PARSE,
      },
  };

  WRAP (parse_type (parser, &dest->create.type, e));

  return SUCCESS;
}

static err_t
parse_query_insert (struct parser *parser, struct query *dest, error *e)
{
  WRAP (parser_expect (parser, TT_INSERT, e));

  if (!parser_match (parser, TT_IDENTIFIER)) {
    return error_causef (e, ERR_SYNTAX, "Expected identifier at position %u", parser->pos);
  }
  struct token *ident = parser_advance (parser);
  ASSERT (ident->type == TT_IDENTIFIER);

  i32 ofst;
  if (!parser_maybe_parse_integer (parser, &ofst)) {
    return error_causef (e, ERR_SYNTAX, "Expected integer at position %u", parser->pos);
  }

  i32 len;
  if (!parser_maybe_parse_integer (parser, &len)) {
    return error_causef (e, ERR_SYNTAX, "Expected integer at position %u", parser->pos);
  }

  *dest = (struct query){
      .type   = QT_INSERT,
      .insert = {
          .name =
              (struct string){
                  .data = (char *)ident->str.data,
                  .len  = ident->str.len,
              },
          .ofst = ofst,
          .len  = len,
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
  if (!parser_match (parser, TT_IDENTIFIER)) {
    return error_causef (e, ERR_SYNTAX, "Expected identifier at position %u", parser->pos);
  }
  struct token      *tok  = parser_advance (parser);

  // USER_STRIDE
  struct user_stride ustr = ustride ();

  if (parser_match (parser, TT_LEFT_BRACKET)) {
    WRAP (parse_user_stride (parser, &ustr, e));
  }

  // LIMIT / BLIMIT
  u32  limit  = 0;
  bool blimit = false;
  if (parser_match (parser, TT_LIMIT)) {
    parser_advance (parser);
    if (!parser_match (parser, TT_INTEGER)) {
      return error_causef (e, ERR_SYNTAX, "Expected number after limit");
    }
    limit  = parser_advance (parser)->integer;
    blimit = false;
  } else if (parser_match (parser, TT_BLIMIT)) {
    parser_advance (parser);
    if (!parser_match (parser, TT_INTEGER)) {
      return error_causef (e, ERR_SYNTAX, "Expected number after limit");
    }
    limit  = parser_advance (parser)->integer;
    blimit = true;
  }

  *dest = (struct query){
      .type = QT_READ,
      .read = {
          .name =
              (struct string){
                  .data = (char *)tok->str.data,
                  .len  = tok->str.len,
              },
          .ustr   = ustr,
          .limit  = limit,
          .blimit = blimit,
      },
  };

  return SUCCESS;
}

static err_t
parse_query_remove (struct parser *parser, struct query *dest, error *e)
{
  // READ
  WRAP (parser_expect (parser, TT_REMOVE, e));

  // IDENT
  if (!parser_match (parser, TT_IDENTIFIER)) {
    return error_causef (e, ERR_SYNTAX, "Expected identifier at position %u", parser->pos);
  }
  struct token      *tok  = parser_advance (parser);

  // USER_STRIDE
  struct user_stride ustr = ustride ();

  if (parser_match (parser, TT_LEFT_BRACKET)) {
    WRAP (parse_user_stride (parser, &ustr, e));
  }

  // LIMIT / BLIMIT
  u32  limit  = 0;
  bool blimit = false;
  if (parser_match (parser, TT_LIMIT)) {
    parser_advance (parser);
    if (!parser_match (parser, TT_INTEGER)) {
      return error_causef (e, ERR_SYNTAX, "Expected number after limit");
    }
    limit  = parser_advance (parser)->integer;
    blimit = false;
  } else if (parser_match (parser, TT_BLIMIT)) {
    parser_advance (parser);
    if (!parser_match (parser, TT_INTEGER)) {
      return error_causef (e, ERR_SYNTAX, "Expected number after limit");
    }
    limit  = parser_advance (parser)->integer;
    blimit = true;
  }

  *dest = (struct query){
      .type   = QT_REMOVE,
      .remove = {
          .name =
              (struct string){
                  .data = (char *)tok->str.data,
                  .len  = tok->str.len,
              },
          .ustr   = ustr,
          .limit  = limit,
          .blimit = blimit,
      },
  };

  return SUCCESS;
}

static err_t
parse_query_write (struct parser *parser, struct query *dest, error *e)
{
  // READ
  WRAP (parser_expect (parser, TT_WRITE, e));

  // IDENT
  if (!parser_match (parser, TT_IDENTIFIER)) {
    return error_causef (e, ERR_SYNTAX, "Expected identifier at position %u", parser->pos);
  }
  struct token      *tok  = parser_advance (parser);

  // USER_STRIDE
  struct user_stride ustr = ustride ();

  if (parser_match (parser, TT_LEFT_BRACKET)) {
    WRAP (parse_user_stride (parser, &ustr, e));
  }

  // LIMIT / BLIMIT
  u32  limit  = 0;
  bool blimit = false;
  if (parser_match (parser, TT_LIMIT)) {
    parser_advance (parser);
    if (!parser_match (parser, TT_INTEGER)) {
      return error_causef (e, ERR_SYNTAX, "Expected number after limit");
    }
    limit  = parser_advance (parser)->integer;
    blimit = false;
  } else if (parser_match (parser, TT_BLIMIT)) {
    parser_advance (parser);
    if (!parser_match (parser, TT_INTEGER)) {
      return error_causef (e, ERR_SYNTAX, "Expected number after limit");
    }
    limit  = parser_advance (parser)->integer;
    blimit = true;
  }

  *dest = (struct query){
      .type  = QT_WRITE,
      .write = {
          .name =
              (struct string){
                  .data = (char *)tok->str.data,
                  .len  = tok->str.len,
              },
          .ustr   = ustr,
          .limit  = limit,
          .blimit = blimit,
      },
  };

  return SUCCESS;
}

err_t
parse_query (struct parser *parser, struct query *dest, struct allocator *dalloc, error *e)
{
  if (parser_match (parser, TT_HELP)) {
    WRAP (parse_query_help (parser, dest, e));
  } else if (parser_match (parser, TT_EXIT)) {
    WRAP (parse_query_exit (parser, dest, e));
  } else if (parser_match (parser, TT_GET)) {
    WRAP (parse_query_get (parser, dest, e));
  } else if (parser_match (parser, TT_DELETE)) {
    WRAP (parse_query_delete (parser, dest, e));
  } else if (parser_match (parser, TT_CREATE)) {
    WRAP (parse_query_create (parser, dest, e));
  } else if (parser_match (parser, TT_INSERT)) {
    WRAP (parse_query_insert (parser, dest, e));
  } else if (parser_match (parser, TT_READ)) {
    WRAP (parse_query_read (parser, dest, e));
  } else if (parser_match (parser, TT_REMOVE)) {
    WRAP (parse_query_remove (parser, dest, e));
  } else if (parser_match (parser, TT_WRITE)) {
    WRAP (parse_query_write (parser, dest, e));
  } else {
    return error_causef (e, ERR_SYNTAX, "Expected a valid operation at pos: %u", parser->pos);
  }

  return SUCCESS;
}

err_t
compile_query (struct query *dest, const char *text, struct allocator *dalloc, error *e)
{
  BUILDER_INIT (b, dalloc);

  struct lexer lex;
  if (lex_tokens (text, &b.temp, strlen (text), &lex, e)) {
    goto theend;
  }

  struct parser parser = parser_init (lex.tokens, &b, lex.ntokens);

  if (parse_query (&parser, dest, dalloc, e)) {
    goto theend;
  }

theend:
  BUILDER_CLOSE (b);
  return error_trace (e);
}

#ifdef TESTING

static void
test_query_green_path (const char *query, struct query expected)
{
  ALLOC_INIT (alloc);

  struct query actual;
  error        e = error_create ();

  TEST_CASE ("SHOULD PASS: %s", query)
  {
    test_assert (compile_query (&actual, query, &alloc, &e) == SUCCESS);
    test_assert (query_equal (&actual, &expected));
  }

  ALLOC_CLOSE (alloc);
}

static void
test_query_red_path (const char *query, err_t code)
{
  ALLOC_INIT (alloc);

  struct query actual;
  error        e = error_create ();

  TEST_CASE ("SHOULD FAIL: %s", query)
  {
    test_err_t_check (compile_query (&actual, query, &alloc, &e), code, &e);
  }

  ALLOC_CLOSE (alloc);
}

TEST (compile_query)
{
  // READ
  {
    test_query_red_path ("read", ERR_SYNTAX);
    test_query_red_path ("read limit", ERR_SYNTAX);
    test_query_red_path ("read blimit", ERR_SYNTAX);
    test_query_red_path ("read 10", ERR_SYNTAX);
    test_query_red_path ("read foo limit", ERR_SYNTAX);
    test_query_red_path ("read foo blimit", ERR_SYNTAX);
    test_query_red_path ("read foo limit -10", ERR_SYNTAX);
    test_query_red_path ("read foo blimit -10", ERR_SYNTAX);
    test_query_red_path ("read foo[0:10] limit -10", ERR_SYNTAX);
    test_query_red_path ("read foo[0:10] blimit -10", ERR_SYNTAX);
    test_query_green_path (
        "read foo",
        (struct query){
            .type = QT_READ,
            .read = {
                .name  = strfcstr ("foo"),
                .ustr  = ustride (),
                .limit = -1,
            },
        }
    );
    test_query_green_path (
        "read foo limit 10",
        (struct query){
            .type = QT_READ,
            .read = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride (),
                .limit  = 10,
                .blimit = false,
            },
        }
    );
    test_query_green_path (
        "read foo blimit 10",
        (struct query){
            .type = QT_READ,
            .read = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride (),
                .limit  = 10,
                .blimit = true,
            },
        }
    );
    test_query_green_path (
        "read foo[0:10:20]",
        (struct query){
            .type = QT_READ,
            .read = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride012 (0, 10, 20),
                .limit  = -1,
                .blimit = false,
            },
        }
    );
    test_query_green_path (
        "read foo[0:10:20] limit 40",
        (struct query){
            .type = QT_READ,
            .read = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride012 (0, 10, 20),
                .limit  = 40,
                .blimit = false,
            },
        }
    );
    test_query_green_path (
        "read foo[0:10:20] blimit 40",
        (struct query){
            .type = QT_READ,
            .read = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride012 (0, 10, 20),
                .limit  = 40,
                .blimit = true,
            },
        }
    );
  }

  // REMOVE
  {
    test_query_red_path ("remove", ERR_SYNTAX);
    test_query_red_path ("remove limit", ERR_SYNTAX);
    test_query_red_path ("remove blimit", ERR_SYNTAX);
    test_query_red_path ("remove 10", ERR_SYNTAX);
    test_query_red_path ("remove foo limit", ERR_SYNTAX);
    test_query_red_path ("remove foo blimit", ERR_SYNTAX);
    test_query_red_path ("remove foo limit -10", ERR_SYNTAX);
    test_query_red_path ("remove foo blimit -10", ERR_SYNTAX);
    test_query_red_path ("remove foo[0:10] limit -10", ERR_SYNTAX);
    test_query_red_path ("remove foo[0:10] blimit -10", ERR_SYNTAX);
    test_query_green_path (
        "remove foo",
        (struct query){
            .type   = QT_REMOVE,
            .remove = {
                .name  = strfcstr ("foo"),
                .ustr  = ustride (),
                .limit = -1,
            },
        }
    );
    test_query_green_path (
        "remove foo limit 10",
        (struct query){
            .type   = QT_REMOVE,
            .remove = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride (),
                .limit  = 10,
                .blimit = false,
            },
        }
    );
    test_query_green_path (
        "remove foo blimit 10",
        (struct query){
            .type   = QT_REMOVE,
            .remove = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride (),
                .limit  = 10,
                .blimit = true,
            },
        }
    );
    test_query_green_path (
        "remove foo[0:10:20]",
        (struct query){
            .type   = QT_REMOVE,
            .remove = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride012 (0, 10, 20),
                .limit  = -1,
                .blimit = false,
            },
        }
    );
    test_query_green_path (
        "remove foo[0:10:20] limit 40",
        (struct query){
            .type   = QT_REMOVE,
            .remove = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride012 (0, 10, 20),
                .limit  = 40,
                .blimit = false,
            },
        }
    );
    test_query_green_path (
        "remove foo[0:10:20] blimit 40",
        (struct query){
            .type   = QT_REMOVE,
            .remove = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride012 (0, 10, 20),
                .limit  = 40,
                .blimit = true,
            },
        }
    );
  }

  // WRITE
  {
    test_query_red_path ("write", ERR_SYNTAX);
    test_query_red_path ("write limit", ERR_SYNTAX);
    test_query_red_path ("write blimit", ERR_SYNTAX);
    test_query_red_path ("write 10", ERR_SYNTAX);
    test_query_red_path ("write foo limit", ERR_SYNTAX);
    test_query_red_path ("write foo blimit", ERR_SYNTAX);
    test_query_red_path ("write foo limit -10", ERR_SYNTAX);
    test_query_red_path ("write foo blimit -10", ERR_SYNTAX);
    test_query_red_path ("write foo[0:10] limit -10", ERR_SYNTAX);
    test_query_red_path ("write foo[0:10] blimit -10", ERR_SYNTAX);
    test_query_green_path (
        "write foo",
        (struct query){
            .type  = QT_WRITE,
            .write = {
                .name  = strfcstr ("foo"),
                .ustr  = ustride (),
                .limit = -1,
            },
        }
    );
    test_query_green_path (
        "write foo limit 10",
        (struct query){
            .type  = QT_WRITE,
            .write = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride (),
                .limit  = 10,
                .blimit = false,
            },
        }
    );
    test_query_green_path (
        "write foo blimit 10",
        (struct query){
            .type  = QT_WRITE,
            .write = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride (),
                .limit  = 10,
                .blimit = true,
            },
        }
    );
    test_query_green_path (
        "write foo[0:10:20]",
        (struct query){
            .type  = QT_WRITE,
            .write = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride012 (0, 10, 20),
                .limit  = -1,
                .blimit = false,
            },
        }
    );
    test_query_green_path (
        "write foo[0:10:20] limit 40",
        (struct query){
            .type  = QT_WRITE,
            .write = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride012 (0, 10, 20),
                .limit  = 40,
                .blimit = false,
            },
        }
    );
    test_query_green_path (
        "write foo[0:10:20] blimit 40",
        (struct query){
            .type  = QT_WRITE,
            .write = {
                .name   = strfcstr ("foo"),
                .ustr   = ustride012 (0, 10, 20),
                .limit  = 40,
                .blimit = true,
            },
        }
    );
  }

  // INSERT
  {
    test_query_red_path ("insert", ERR_SYNTAX);
    test_query_red_path ("insert foo", ERR_SYNTAX);
    test_query_red_path ("insert foo 0", ERR_SYNTAX);
    test_query_green_path (
        "insert foo 0 1",
        (struct query){
            .type   = QT_INSERT,
            .insert = {.name = strfcstr ("foo"), .ofst = 0, .len = 1},
        }
    );
  }

  // CREATE
  {
    test_query_red_path ("create", ERR_SYNTAX);
    test_query_red_path ("create 1", ERR_SYNTAX);
    test_query_red_path ("create foo", ERR_SYNTAX);
    test_query_red_path ("create foo 1", ERR_SYNTAX);

    // Sarray
    u32           dims[2]   = {10, 20};
    struct type   t0        = {.type = T_SARRAY, .sa = {.rank = 2, .dims = dims, .t = &TF32}};

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
        "create foo struct { a i32, b union { c i32, d [10][20]f32 } }",
        (struct query){
            .type   = QT_CREATE,
            .create = {
                .name = strfcstr ("foo"),
                .type = t2,
            },
        }
    );
  }

  // DELETE
  {
    test_query_red_path ("delete", ERR_SYNTAX);
    test_query_red_path ("delete 1", ERR_SYNTAX);
    test_query_green_path (
        "delete foo",
        (struct query){
            .type   = QT_DELETE,
            .delete = {
                .name = strfcstr ("foo"),
            },
        }
    );
  }

  // GET
  {
    test_query_red_path ("get", ERR_SYNTAX);
    test_query_red_path ("get 1", ERR_SYNTAX);
    test_query_red_path ("get if", ERR_SYNTAX);
    test_query_red_path ("get if foo", ERR_SYNTAX);
    test_query_green_path (
        "get foo",
        (struct query){
            .type = QT_GET,
            .get  = {
                .name      = strfcstr ("foo"),
                .if_exists = false,
            },
        }
    );
    test_query_green_path (
        "get if exists foo",
        (struct query){
            .type = QT_GET,
            .get  = {
                .name      = strfcstr ("foo"),
                .if_exists = true,
            },
        }
    );
  }

  {
    test_query_green_path (
        "exit",
        (struct query){
            .type = QT_EXIT,
        }
    );
  }

  {
    test_query_green_path (
        "help",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = false},
        }
    );
    test_query_green_path (
        "help read",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_READ},
        }
    );
    test_query_green_path (
        "help create",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_CREATE},
        }
    );
    test_query_green_path (
        "help delete",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_DELETE},
        }
    );
    test_query_green_path (
        "help get",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_GET},
        }
    );
    test_query_green_path (
        "help exit",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_EXIT},
        }
    );
    test_query_green_path (
        "help help",
        (struct query){
            .type = QT_HELP,
            .help = {.has_command = true, .command = QT_HELP},
        }
    );
  }
}
#endif
