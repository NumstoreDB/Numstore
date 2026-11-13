Inspect Variables
=================

This guide shows how to read the metadata of one variable, or of all
variables. For Python, see `get`, `get_all` and `Var` in the
[Python API reference](../reference/python_api.md#get-and-get_all).

In C
----

`ns_get_var` gives you a handle to the metadata of one variable. Any query that
names a variable works. The query does not run. So you can ask what a
destructive query *would* touch. Release the handle with `ns_var_free`.
`ns_visit_nsdb_vars` goes through all variables in the database:

```c
#include "numstore.h"

#include <inttypes.h>
#include <stdio.h>

struct total
{
  unsigned seen;
  b_size   elements;
};

static void
tally (nsdb_var_t *var, void *ctx)
{
  struct total *t = ctx;
  t->seen++;
  t->elements += ns_var_len (var);
  // Do not call ns_var_free here: the visitor owns this handle.
}

int
main (void)
{
  ns_cleanup ("vars.db");
  nsdb_t *db = ns_open ("vars.db");

  ns_execute (db, NULL, "create v1 i32");
  ns_execute (db, NULL, "create v2 f64");

  const int32_t src[] = {1, 2, 3};
  ns_write (db, NULL, src, sizeof (src), "insert v1 0 3");

  // One variable at a time.
  nsdb_var_t *var = ns_get_var (db, NULL, "get v1");
  if (var == NULL) {
    fprintf (stderr, "get failed: %s\n", ns_strerror (db, false));
    ns_close (db);
    return 1;
  }
  printf ("v1 has %" PRIu64 " elements\n", ns_var_len (var));
  ns_var_free (var);

  // Any query that names a variable works, and the query never runs.
  var = ns_get_var (db, NULL, "remove v1[0:]");
  printf ("v1 still has %" PRIu64 " elements\n", ns_var_len (var));
  ns_var_free (var);

  // Every variable in the database.
  struct total t = {0};
  if (ns_visit_nsdb_vars (db, tally, NULL, &t) != 0) {
    fprintf (stderr, "visit failed: %s\n", ns_strerror (db, false));
    ns_close (db);
    return 1;
  }
  printf ("%u variables, %" PRIu64 " elements total\n", t.seen, t.elements);

  return ns_close (db);
}
```

```text
v1 has 3 elements
v1 still has 3 elements
2 variables, 3 elements total
```

Ownership rules
---------------

* A handle from `ns_get_var` is yours. Release it with `ns_var_free`. Each
  variable has its own memory. So you can release it before or after
  `ns_close`.
* A handle that `ns_visit_nsdb_vars` gives to the callback is **not** yours. Do
  not release it. Do not keep it after the callback. Copy the data that you
  need.

The transaction argument
------------------------

`ns_visit_nsdb_vars` has a transaction argument, but it does not use it now.
The walk always sees the committed state.

What C can see
--------------

In C, `ns_var_len` is all of the metadata: the length in elements. The C API
has no function for the type or the element size. To get those, use the type
that you wrote in the `create` query. Or use
[`Var` in Python](../reference/python_api.md#var).
