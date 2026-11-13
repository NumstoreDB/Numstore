Inspect Variables
=================

How to look at one variable's metadata, or every variable's. In Python that's
`db.get`, `db.get_all` and `Var` - see
[the query language tutorial](../tutorials/language_tour.md#variable-operations).

In C
----

`ns_get_var` hands you a handle to one variable's metadata. Any query that names
a variable works and the query itself never runs, so you can ask what a
destructive query *would* touch. Free the handle with `ns_var_free`;
`ns_visit_nsdb_vars` walks every variable in the database instead:

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

Two ownership rules worth keeping straight:

* A handle from `ns_get_var` is yours, and `ns_var_free` releases it. Variables
  get their own memory, so you may free them before or after `ns_close`.
* A handle passed to a `ns_visit_nsdb_vars` callback is **not** yours. Don't
  free it, and don't keep it past the callback - copy out what you need.

The transaction argument
------------------------

`ns_visit_nsdb_vars` takes a transaction argument for symmetry but ignores it
today: the walk always sees committed state.

What C can see
--------------

In C, `ns_var_len` is the whole metadata interface - length in elements. The
type and element size are not exposed as C accessors; when you need those, read
them from the type you wrote in the `create` query, or use `Var` in Python.
