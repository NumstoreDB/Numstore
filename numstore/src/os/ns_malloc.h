#ifndef NS_MALLOC_H
#define NS_MALLOC_H

#include "core/ns_error.h"
#include "core/ns_stdtypes.h"

struct i_mem_vtable
{
  void *(*malloc) (void *self, const u32 nelem, const u32 size, error *e);
  void *(*calloc) (void *self, const u32 nelem, const u32 size, error *e);
  void *(*realloc) (void *self, void *ptr, const u32 nelem, const u32 size, error *e);
  void (*free) (void *self, void *v);
};

struct i_mem
{
  const struct i_mem_vtable *table;
  void                      *self;
};

struct i_mem default_mem (void);

#endif
