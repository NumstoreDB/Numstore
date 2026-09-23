#include "core/ns_slab_alloc.h"
#include "nscore/nsdb/ns_nsdb.h"

struct ns_plan *
ns_plan_create (struct nsdb *db, const char *query, error *e)
{
  struct ns_plan *plan = slab_alloc_alloc (&db->plan_alloc, e);
}
