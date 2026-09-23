#include "nscore/plan/ns_plan.h"

#include "nscore/algorithms/numstore/ns_numstore_algorithms.h"
#include "nscore/variables/ns_variables.h"

// Create and free a plan
struct ns_plan *ns_plan_create (struct slab_alloc *alloc, const char *query, error *e);
void ns_plan_free (struct slab_alloc *alloc, struct ns_plan *plan);

// Execute statments
struct numstore_var *
ns_plan_get_var (struct ns_plan *st, ns_txn_t *txn, error *e)
{
  if (st->var != NULL) {
    return st->var;
  }

  switch (st->q.type) {
    // Array Operations
    case QT_READ: {
      st->var = nsdb_var_create (st->mem, e);
      numstore_get (st->p, txn, false, st->q.read.name, &st->alloc, &st->var, error * e)
    }
    case QT_WRITE: {
    }
    case QT_INSERT: {
    }
    case QT_REMOVE: {
    }

    // Variable Operations
    case QT_CREATE: {
    }
    case QT_DELETE: {
    }
    case QT_GET: {
    }

    // Meta Operations
    case QT_EXIT: {
    }
    case QT_HELP: {
    }
  }
}

sb_size
ns_plan_read (struct ns_plan *st, ns_txn_t *txn, void *dest, b_size dlen, error *e)
{}

void *
ns_plan_read_malloc (struct ns_plan *st, ns_txn_t *txn, b_size *dlen, error *e)
{}

sb_size
ns_plan_write (struct ns_plan *st, ns_txn_t *txn, const void *src, b_size dlen, error *e)
{}
