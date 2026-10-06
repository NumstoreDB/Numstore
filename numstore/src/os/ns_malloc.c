#include "os/ns_malloc.h"

#include "core/ns_bounds.h"

#include <errno.h>

void *
i_malloc (void *self, const u32 nelem, const u32 size, error *e)
{
  ASSERT (self == NULL);
  ASSERT (nelem > 0);
  ASSERT (size > 0);

  u32 bytes;
  if (!safe_mul_u32 (&bytes, nelem, size)) {
    error_causef (e, ERR_NOMEM, "malloc %d*%d: overflow", nelem, size);
    return NULL;
  }

  errno     = 0;
  void *ret = malloc ((size_t)bytes);
  if (ret == NULL) {
    if (errno == ENOMEM) {
      error_causef (e, ERR_NOMEM, "malloc %d*%d: %s", nelem, size, strerror (errno));
    } else {
      error_causef (e, ERR_NOMEM, "malloc: %s", strerror (errno));
    }
  }
  return ret;
}

void *
i_calloc (void *self, const u32 nelem, const u32 size, error *e)
{
  ASSERT (self == NULL);
  ASSERT (nelem > 0);
  ASSERT (size > 0);

  u32 bytes = 0;
  if (!safe_mul_u32 (&bytes, nelem, size)) {
    error_causef (e, ERR_NOMEM, "malloc %d*%d: overflow", nelem, size);
    return NULL;
  }

  ASSERT (bytes > 0);

  errno     = 0;
  void *ret = calloc ((size_t)nelem, (size_t)size);
  if (ret == NULL) {
    if (errno == ENOMEM) {
      error_causef (e, ERR_NOMEM, "calloc %d*%d: %s", nelem, size, strerror (errno));
    } else {
      error_causef (e, ERR_NOMEM, "calloc: %s", strerror (errno));
    }
  }
  return ret;
}

void *
i_realloc (void *self, void *ptr, const u32 nelem, const u32 size, error *e)
{
  ASSERT (self == NULL);
  ASSERT (nelem > 0);
  ASSERT (size > 0);

  u32 bytes = 0;
  {
    bool ok = safe_mul_u32 (&bytes, nelem, size);
    ASSERT (ok);
    if (!ok) {
      error_causef (e, ERR_NOMEM, "realloc %u*%u: overflow", nelem, size);
      return NULL;
    }
  }

  errno     = 0;
  void *ret = realloc (ptr, (size_t)bytes);
  if (ret == NULL) {
    error_causef (e, ERR_NOMEM, "realloc %u bytes: %s", bytes, strerror (errno));
    return NULL;
  }
  return ret;
}

void
i_free (void *self, void *v)
{
  ASSERT (self == NULL);
  ASSERT (v);
  free (v);
}

static const struct i_mem_vtable default_mem_vtable = {
    .malloc  = i_malloc,
    .calloc  = i_calloc,
    .realloc = i_realloc,
    .free    = i_free,
};

struct i_mem
default_mem (void)
{
  return (struct i_mem){
      .self  = NULL,
      .table = &default_mem_vtable,
  };
}
