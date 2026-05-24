/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.

#include "mem_vhmap.h"

#include "c_specx.h"
#include "nscore/errors.h"
#include "nscore/types.h"
#include "nscore/variables.h"

struct var_frame
{
  struct variable    var;
  struct hnode       node;
  struct chunk_alloc alloc;
};

// Lifecycle
struct mem_vhmap *
mem_vhmap_create (error *e)
{
  struct mem_vhmap *ret = i_malloc (1, sizeof *ret, e);
  if (ret == NULL) { return NULL; }

  ret->vhasht = htable_create (256, e);
  if (ret->vhasht == NULL)
  {
    i_free (ret);
    return NULL;
  }

  slab_alloc_init (&ret->alloc, sizeof (struct var_frame), 256);

  return ret;
}

static void
var_frame_free (struct hnode *node, void *ctx)
{
  struct var_frame *frame = container_of (node, struct var_frame, node);
  chunk_alloc_free_all (&frame->alloc);
}

void
mem_vhmap_free (struct mem_vhmap *db)
{
  ASSERT (db);
  htable_foreach (db->vhasht, var_frame_free, NULL);
  slab_alloc_destroy (&db->alloc);
  htable_free (db->vhasht);
  i_free (db);
}

struct copy_ctx
{
  struct mem_vhmap *dest;
  error            *e;
};

static void
move_data (struct hnode *node, void *ctx)
{
  struct copy_ctx *_ctx = ctx;
  if (_ctx->e->cause_code < 0) { return; }

  struct var_frame *frame = container_of (node, struct var_frame, node);
  mem_vhmap_add_var (_ctx->dest, &frame->var, _ctx->e);
}

struct mem_vhmap *
mem_vhmap_clone (const struct mem_vhmap *src, error *e)
{
  struct mem_vhmap *ret = mem_vhmap_create (e);
  if (ret == NULL) { return NULL; }

  struct copy_ctx ctx = {
      .dest = ret,
      .e    = e,
  };

  htable_foreach (src->vhasht, move_data, &ctx);

  if (ctx.e->cause_code)
  {
    mem_vhmap_free (ret);
    return NULL;
  }

  return ret;
}

static bool
vframe_eq (const struct hnode *left, const struct hnode *right)
{
  struct var_frame *_left  = container_of (left, struct var_frame, node);
  struct var_frame *_right = container_of (right, struct var_frame, node);
  return string_equal (_left->var.vname, _right->var.vname);
}

// Create
err_t
mem_vhmap_add_var (struct mem_vhmap *db, struct variable *var, error *e)
{
  struct var_frame key = {
      .var = (struct variable){
          .vname = var->vname,
      },
  };
  hnode_init (&key.node, fnv1a_hash (var->vname));
  struct hnode **found = htable_lookup (db->vhasht, &key.node, vframe_eq);
  if (found) { return error_causef (e, ERR_DUPLICATE_VARIABLE, "Variable already exists"); }
  else
  {
    struct var_frame *frame = slab_alloc_alloc (&db->alloc, e);
    if (frame == NULL) { return error_trace (e); }

    chunk_alloc_create_default (&frame->alloc);

    frame->var.vname.data = chunk_alloc_move_mem (
        &frame->alloc,
        var->vname.data,
        var->vname.len + 1, // TODO - this is awful - remove +1
        e
    );
    frame->var.vname.len = var->vname.len;
    frame->var.dtype     = type_movemem (var->dtype, &frame->alloc, e);
    frame->var.var_root  = var->var_root;
    frame->var.rpt_root  = var->rpt_root;
    frame->var.nbytes    = var->nbytes;

    if (frame->var.vname.data == NULL || frame->var.dtype == NULL)
    {
      chunk_alloc_free_all (&frame->alloc);
      slab_alloc_free (&db->alloc, frame);
      return error_trace (e);
    }

    hnode_init (&frame->node, fnv1a_hash (var->vname));
    htable_insert (db->vhasht, &frame->node);

    return SUCCESS;
  }
}

struct variable *
mem_vhmap_get_var (struct mem_vhmap *db, struct string name)
{
  struct var_frame key = {
      .var = (struct variable){
          .vname = name,
      },
  };
  hnode_init (&key.node, fnv1a_hash (name));

  struct hnode **found = htable_lookup (db->vhasht, &key.node, vframe_eq);
  if (found) { return &container_of (*found, struct var_frame, node)->var; }
  else
  {
    return NULL;
  }
}

void
mem_vhmap_remove_var (struct mem_vhmap *db, struct string name)
{
  struct var_frame key = {
      .var = (struct variable){
          .vname = name,
      },
  };
  hnode_init (&key.node, fnv1a_hash (name));

  struct hnode **found = htable_lookup (db->vhasht, &key.node, vframe_eq);
  ASSERT (found);
  struct var_frame *frame = container_of (*found, struct var_frame, node);
  chunk_alloc_free_all (&frame->alloc);
  htable_delete (db->vhasht, found);
}

u32
mem_vhmap_count (struct mem_vhmap *db)
{ return htable_size (db->vhasht); }

struct rand_ctx
{
  u32              index;
  u32              target;
  struct variable *dest;
};

static void
random_iter (struct hnode *node, void *_ctx)
{
  struct rand_ctx *ctx = _ctx;

  // Already done
  if (ctx->dest) { return; }

  if (ctx->index == ctx->target) { ctx->dest = &container_of (node, struct var_frame, node)->var; }

  ctx->index++;
}

struct variable *
mem_vhmap_random (struct mem_vhmap *db)
{
  struct rand_ctx ctx = {
      .index  = 0,
      .target = randu32r (1, MAX (htable_size (db->vhasht), 1)) - 1,
      .dest   = NULL,
  };
  htable_foreach (db->vhasht, random_iter, &ctx);
  return ctx.dest;
}

#ifndef NTEST
TEST (mem_vhmap)
{
  error             e = error_create ();
  struct mem_vhmap *v = mem_vhmap_create (&e);

  struct type deftype = {
      .type = T_PRIM,
      .p    = U32,
  };
  char buf[32];

  TEST_CASE ("every added var is immediately retrievable with correct data")
  {
    for (int i = 0; i < 10000; ++i)
    {
      snprintf (buf, sizeof buf, "var_%d", i);
      struct variable var = {
          .vname  = strfcstr (buf),
          .dtype  = &deftype,
          .nbytes = (b_size)i,
      };
      err_t err = mem_vhmap_add_var (v, &var, &e);
      ASSERT (err == 0);

      struct variable *got = mem_vhmap_get_var (v, strfcstr (buf));
      ASSERT (got != NULL);
      ASSERT (got->nbytes == (b_size)i);
    }
  }

  TEST_CASE ("duplicate add fails")
  {
    {
      struct variable dup = {
          .vname = strfcstr ("var_0"),
          .dtype = &deftype,
      };
      ASSERT (mem_vhmap_add_var (v, &dup, &e) != 0);
      e.cause_code = 0;
      e.cmlen      = 0;
    }
  }

  TEST_CASE ("get on unknown name returns NULL")
  { ASSERT (mem_vhmap_get_var (v, strfcstr ("__no_such_var__")) == NULL); }

  TEST_CASE ("remove makes the var unretrievable; others unaffected")
  {
    for (int i = 0; i < 10000; ++i)
    {
      snprintf (buf, sizeof buf, "var_%d", i);
      struct string name = strfcstr (buf);
      mem_vhmap_remove_var (v, name);
      ASSERT (mem_vhmap_get_var (v, name) == NULL);

      // spot-check that the next var (if any) is still there
      if (i + 1 < 10000)
      {
        snprintf (buf, sizeof buf, "var_%d", i + 1);
        ASSERT (mem_vhmap_get_var (v, strfcstr (buf)) != NULL);
      }
    }
  }

  TEST_CASE ("clone captures state; mutations in clone don't affect original")
  {
    for (int i = 0; i < 128; ++i)
    {
      snprintf (buf, sizeof buf, "cv_%d", i);
      struct variable var = {
          .vname  = strfcstr (buf),
          .nbytes = (b_size)(i * 3),
          .dtype  = &deftype,
      };
      mem_vhmap_add_var (v, &var, &e);
    }

    struct mem_vhmap *c = mem_vhmap_clone (v, &e);
    ASSERT (c != NULL);

    for (int i = 0; i < 128; ++i)
    {
      snprintf (buf, sizeof buf, "cv_%d", i);
      struct variable *got = mem_vhmap_get_var (c, strfcstr (buf));
      ASSERT (got != NULL);
      ASSERT (got->nbytes == (b_size)(i * 3));
    }

    mem_vhmap_remove_var (c, strfcstr ("cv_0"));
    ASSERT (mem_vhmap_get_var (c, strfcstr ("cv_0")) == NULL); // removed in clone
    ASSERT (mem_vhmap_get_var (v, strfcstr ("cv_0")) != NULL); // original intact

    mem_vhmap_free (c);
  }

  mem_vhmap_free (v);
}
#endif
