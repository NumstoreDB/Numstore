Use Transactions in C
=====================

`ns_begin` opens a transaction; pass it where you were passing `NULL` and the
queries become one atomic unit. `ns_commit` makes it durable, `ns_rollback`
throws it away, and reads inside the transaction see its own uncommitted work:

```c
#include "numstore.h"

#include <inttypes.h>
#include <stdio.h>

static void
print_prices (nsdb_t *db, const char *label)
{
  double  out[8];
  sb_size n = ns_read (db, NULL, out, sizeof (out), "read prices[0:]");
  printf ("%s:", label);
  for (sb_size i = 0; i < n; i++) {
    printf (" %g", out[i]);
  }
  printf ("\n");
}

int
main (void)
{
  ns_cleanup ("txn.db");
  nsdb_t *db = ns_open ("txn.db");
  ns_execute (db, NULL, "create prices f64");

  // Commit: both inserts land together, or neither does.
  nsdb_txn_t *tx = ns_begin (db);
  if (tx == NULL) {
    fprintf (stderr, "begin failed: %s\n", ns_strerror (db, false));
    ns_close (db);
    return 1;
  }

  const double first[] = {1.5, 2.25};
  const double second[] = {3.75};
  ns_write (db, tx, first, sizeof (first), "insert prices 0 2");
  ns_write (db, tx, second, sizeof (second), "insert prices 2 1");

  if (ns_commit (db, tx) != 0) {
    fprintf (stderr, "commit failed: %s\n", ns_strerror (db, false));
    ns_close (db);
    return 1;
  }
  print_prices (db, "after commit ");

  // Rollback: the insert is thrown away.
  tx = ns_begin (db);
  const double third[] = {99.0};
  ns_write (db, tx, third, sizeof (third), "insert prices 0 1");

  double  inside[8];
  sb_size n = ns_read (db, tx, inside, sizeof (inside), "read prices[0:]");
  printf ("inside txn  : %" PRId64 " elements, first %g\n", n, inside[0]);

  ns_rollback (db, tx);
  print_prices (db, "after rollback");

  return ns_close (db);
}
```

```text
after commit : 1.5 2.25 3.75
inside txn  : 4 elements, first 99
after rollback: 1.5 2.25 3.75
```

Rules
-----

* A `nsdb_txn_t *` is spent once you commit or roll it back - don't reuse it.
* Finish every transaction you open: `ns_close` blocks while one is still open,
  and `ns_begin` blocks while another is still open. See
  [Transactions and concurrency](../concepts/transactions_and_concurrency.md).
