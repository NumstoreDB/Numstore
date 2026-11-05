# C: Use Prepared Plans

`ns_execute`, `ns_read` and `ns_write` compile the query string, run it, and
throw the compiled form away. A plan keeps it, so a query you run in a loop is
compiled once:

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
    fprintf (stderr, "plan failed: %s\n", ns_strerror (db));
    ns_close (db);
    return 1;
  }

  // Run it many times.
  for (int batch = 0; batch < 3; batch++) {
    const uint32_t src[] = {1, 2, 3, 4};
    sb_size        n = ns_plan_write (append, NULL, src, sizeof (src));
    if (n < 0) {
      fprintf (stderr, "write failed: %s\n", ns_plan_strerror (append));
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

Each plan function mirrors the one-shot function, with the transaction as an
argument so one compiled plan can run inside any transaction:

| One shot        | Prepared              |
|-----------------|-----------------------|
| `ns_execute`    | `ns_plan_execute`     |
| `ns_read`       | `ns_plan_read`        |
| `ns_write`      | `ns_plan_write`       |
| `ns_malloc`     | `ns_plan_malloc`      |
| `ns_get_var`    | `ns_plan_get_var`     |
| `ns_strerror`   | `ns_plan_strerror`    |

Plans must be freed before the database closes.
