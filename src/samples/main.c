#include "numstore.h"

int
main ()
{
  nsdb_t *db = nsdb_open ("foo");
  nsdb_close (db);
}
