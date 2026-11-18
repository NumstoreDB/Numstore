#include "numstore.h"

#include <stdio.h>

int
main (void)
{
  // Start with a new file
  ns_cleanup ("prices.db");
  nsdb_t *db = ns_open ("prices.db");
  if (db == NULL) {
    return 1;
  }

  // Make a variable. Its elements are f64 numbers.
  if (ns_execute (db, NULL, "create prices f64") != 0) {
    fprintf (stderr, "%s\n", ns_strerror (db, false));
    return 1;
  }

  // Insert 3 numbers at index 0
  double in[] = {1.5, 2.25, 3.75};
  if (ns_write (db, NULL, in, sizeof (in), "insert prices 0 3") < 0) {
    fprintf (stderr, "%s\n", ns_strerror (db, false));
    return 1;
  }

  // Insert 1 number in the middle
  double middle = 9.0;
  if (ns_write (db, NULL, &middle, sizeof (middle), "insert prices 1 1") < 0) {
    fprintf (stderr, "%s\n", ns_strerror (db, false));
    return 1;
  }

  // Read all of the numbers
  double  out[8];
  sb_size n = ns_read (db, NULL, out, sizeof (out), "read prices");
  for (sb_size i = 0; i < n; i++) {
    printf ("%g\n", out[i]);
  }

  ns_close (db);
  return 0;
}
