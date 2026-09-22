/// ns_dims_vec.h — tiny growable npy_intp array used when flattening
/// nested T_SARRAY shapes into a single numpy dims[] buffer.

#include "ns_pynumstore.h"

#include <Python.h>

void
dims_vec_create (struct dims_vec *v)
{
  v->data = NULL;
  v->len  = 0;
  v->cap  = 0;
}

int
dims_vec_append (struct dims_vec *v, npy_intp dim)
{
  if (v->len == v->cap) {
    int       new_cap = (v->cap == 0) ? 4 : v->cap * 2;

    npy_intp *tmp     = PyMem_Realloc (v->data, (size_t)new_cap * sizeof (npy_intp));
    if (tmp == NULL) {
      PyErr_NoMemory ();
      return -1;
    }

    v->data = tmp;
    v->cap  = new_cap;
  }

  v->data[v->len++] = dim;
  return 0;
}

int
dims_vec_append_many (struct dims_vec *v, const u32 *dims, u32 count)
{
  for (u32 i = 0; i < count; i++) {
    if (dims_vec_append (v, (npy_intp)dims[i]) != 0) {
      return -1;
    }
  }
  return 0;
}

void
dims_vec_free (struct dims_vec *v)
{
  PyMem_Free (v->data);
  v->data = NULL;
  v->len  = 0;
  v->cap  = 0;
}
