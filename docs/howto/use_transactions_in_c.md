Use Transactions in C
=====================

`ns_begin` opens a transaction. Give it to each query in place of `NULL`. Then
the queries are one atomic unit. `ns_commit` makes the work durable.
`ns_rollback` discards the work. A read in the transaction sees the work of the
transaction that is not committed:

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

* After you commit a `nsdb_txn_t *` or roll it back, you cannot use it again.
* End each transaction that you open. `ns_close` waits while a transaction is
  open. `ns_begin` waits while another transaction is open. See
  [Transactions and concurrency](../concepts/transactions_and_concurrency.md).
