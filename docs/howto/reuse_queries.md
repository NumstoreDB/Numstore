Reuse a Query with a Plan
=========================

Each query call compiles the query string, runs it, and then discards the
compiled form. A plan keeps the compiled form. So a query that you run in a
loop is compiled one time only.

In Python
---------

`db.prepare` makes a plan. This example uses a `u32` variable `out` that holds
`[1, 2, 3, 4]`:

```python
>>> db.execute("create out u32")
>>> db.write("insert out 0 4", [1, 2, 3, 4])
4
>>> plan = db.prepare("read out[0:2]")
>>> plan
<Plan 'read out[0:2]'>
>>> plan.read()
array([1, 2], dtype=uint32)
>>> plan.read()
array([1, 2], dtype=uint32)
>>> plan.close()
```

A `Plan` has the methods `execute`, `read`, `read_into`, `write` and `var`.
Each method takes a transaction as an optional last argument. A `Plan` is also
a context manager:

```python
>>> with db.prepare("insert out 0 2") as plan:
...     with db.begin() as txn:
...         plan.write([10, 20], txn)
2
>>> db.read("read out")
array([10, 20,  1,  2,  3,  4], dtype=uint32)
```

The [Python API reference](../reference/python_api.md#plan) gives all members
of `Plan`.

In C
----

`ns_plan_fcreate` makes a plan:

```c
#include "numstore.h"

#include <inttypes.h>
#include <stdio.h>

int
main (void)
{
  ns_cleanup ("plans.db");
  nsdb_t *db = ns_open ("plans.db");
  ns_execute (db, NULL, "create samples u32");

  // Compile once.
  nsdb_plan_t *append = ns_plan_fcreate (db, "insert samples 0 4");
  if (append == NULL) {
    fprintf (stderr, "plan failed: %s\n", ns_strerror (db, false));
    ns_close (db);
    return 1;
  }

  // Run it many times.
  for (int batch = 0; batch < 3; batch++) {
    const uint32_t src[] = {1, 2, 3, 4};
    sb_size        n = ns_plan_write (append, NULL, src, sizeof (src));
    if (n < 0) {
      fprintf (stderr, "write failed: %s\n", ns_plan_strerror (append, false));
      ns_plan_free (append);
      ns_close (db);
      return 1;
    }
    printf ("batch %d wrote %" PRId64 "\n", batch, n);
  }

  nsdb_var_t *var = ns_plan_get_var (append, NULL);
  printf ("samples now holds %" PRIu64 " elements\n", ns_var_len (var));
  ns_var_free (var);

  ns_plan_free (append); // plans must be freed before the database closes

  nsdb_plan_t *head = ns_plan_fcreate (db, "read samples[0:4]");
  uint32_t     out[4];
  sb_size      n = ns_plan_read (head, NULL, out, sizeof (out));
  printf ("first %" PRId64 ":", n);
  for (sb_size i = 0; i < n; i++) {
    printf (" %u", out[i]);
  }
  printf ("\n");
  ns_plan_free (head);

  return ns_close (db);
}
```

```text
batch 0 wrote 4
batch 1 wrote 4
batch 2 wrote 4
samples now holds 12 elements
first 4: 1 2 3 4
```

Each plan function matches a one-shot function. The plan function takes the
transaction as an argument. So one compiled plan can run in any transaction:

| One shot      | Prepared           |
| ------------- | ------------------ |
| `ns_execute`  | `ns_plan_execute`  |
| `ns_read`     | `ns_plan_read`     |
| `ns_write`    | `ns_plan_write`    |
| `ns_malloc`   | `ns_plan_malloc`   |
| `ns_get_var`  | `ns_plan_get_var`  |
| `ns_strerror` | `ns_plan_strerror` |

Release each plan before the database closes.
