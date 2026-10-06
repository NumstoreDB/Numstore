/// Copyright 2026 Theo Lincke
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

/// Faulty OS: a decorator over any i_os.
///
/// Every fallible call rolls first and fails with probability
/// ctx->fail_percent, otherwise it forwards to the delegate OS.
///
/// Every object handed out (file, thread, mutex, cond, timer) has a .self
/// that points to a small wrapper holding the delegate's {table, self} plus a
/// back pointer to the ctx. Object-level ops (pread, lock, now_ns...) only
/// receive that .self, so the wrapper is the only place they can find the
/// delegate table and the fail rate. Wrappers come from per-kind slabs and
/// are released on close / join / free.

#include "core/ns_csx_assert.h"
#include "core/ns_error.h"
#include "core/ns_numerics.h"
#include "core/ns_slab_alloc.h"
#include "os/ns_os.h"

#include <stdbool.h>

////////////////////////////////////////////////////////////
// Types

struct faulty_ctx
{
  struct i_os       delegate;
  struct i_mem      mem;
  float             fail_percent;

  struct slab_alloc file_alloc;
  struct slab_alloc thread_alloc;
  struct slab_alloc mutex_alloc;
  struct slab_alloc cond_alloc;
  struct slab_alloc timer_alloc;
};

struct faulty_file
{
  struct i_file      d;
  struct faulty_ctx *ctx;
};

struct faulty_thread
{
  struct i_thread    d;
  struct faulty_ctx *ctx;
};

struct faulty_mutex
{
  struct i_mutex     d;
  struct faulty_ctx *ctx;
};

struct faulty_cond
{
  struct i_cond      d;
  struct faulty_ctx *ctx;
};

struct faulty_timer
{
  struct i_timer     d;
  struct faulty_ctx *ctx;
};

static const struct os_vtable faulty_os_vtable;

////////////////////////////////////////////////////////////
// Debug asserts

DEFINE_DBG_ASSERT (struct faulty_ctx, faulty_ctx, c, {
  ASSERT (c);
  ASSERT (c->delegate.table);
  ASSERT (c->mem.table);
  ASSERT (c->fail_percent >= 0.0f && c->fail_percent <= 1.0f);
})

DEFINE_DBG_ASSERT (struct faulty_file, faulty_file, f, {
  ASSERT (f);
  ASSERT (f->ctx);
  ASSERT (f->d.table);
  ASSERT (f->d.table == f->ctx->delegate.table);
})

DEFINE_DBG_ASSERT (struct faulty_thread, faulty_thread, t, {
  ASSERT (t);
  ASSERT (t->ctx);
  ASSERT (t->d.table);
  ASSERT (t->d.table == t->ctx->delegate.table);
})

DEFINE_DBG_ASSERT (struct faulty_mutex, faulty_mutex, m, {
  ASSERT (m);
  ASSERT (m->ctx);
  ASSERT (m->d.table);
  ASSERT (m->d.table == m->ctx->delegate.table);
})

DEFINE_DBG_ASSERT (struct faulty_cond, faulty_cond, c, {
  ASSERT (c);
  ASSERT (c->ctx);
  ASSERT (c->d.table);
  ASSERT (c->d.table == c->ctx->delegate.table);
})

DEFINE_DBG_ASSERT (struct faulty_timer, faulty_timer, t, {
  ASSERT (t);
  ASSERT (t->ctx);
  ASSERT (t->d.table);
  ASSERT (t->d.table == t->ctx->delegate.table);
})

////////////////////////////////////////////////////////////
// Utils

static inline err_t
faulty_roll (const struct faulty_ctx *ctx, error *e, err_t type)
{
  DBG_ASSERT (faulty_ctx, ctx);

  if (randf () < ctx->fail_percent) {
    return error_causef (e, type, "Injected fault");
  }
  return SUCCESS;
}

////////////////////////////////////////////////////////////
// Files

typedef err_t (*open_fn) (void *os_self, i_file *dest, const char *fname, error *e);

static err_t
faulty_open_file (struct faulty_ctx *ctx, open_fn open, i_file *dest, const char *fname, error *e)
{
  DBG_ASSERT (faulty_ctx, ctx);
  ASSERT (open);
  ASSERT (dest);
  ASSERT (fname);

  if (faulty_roll (ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  struct faulty_file *f = slab_alloc_alloc (&ctx->file_alloc, e);
  if (f == NULL) {
    return error_trace (e);
  }

  if (open (ctx->delegate.self, &f->d, fname, e)) {
    slab_alloc_free (&ctx->file_alloc, f);
    return error_trace (e);
  }

  f->ctx = ctx;
  DBG_ASSERT (faulty_file, f);

  dest->table = &faulty_os_vtable;
  dest->self  = f;

  return SUCCESS;
}

static err_t
faulty_open_file_rw (void *os_self, i_file *dest, const char *fname, error *e)
{
  struct faulty_ctx *ctx = os_self;
  DBG_ASSERT (faulty_ctx, ctx);
  return faulty_open_file (ctx, ctx->delegate.table->open_file_rw, dest, fname, e);
}

static err_t
faulty_open_file_r (void *os_self, i_file *dest, const char *fname, error *e)
{
  struct faulty_ctx *ctx = os_self;
  DBG_ASSERT (faulty_ctx, ctx);
  return faulty_open_file (ctx, ctx->delegate.table->open_file_r, dest, fname, e);
}

static err_t
faulty_open_file_w (void *os_self, i_file *dest, const char *fname, error *e)
{
  struct faulty_ctx *ctx = os_self;
  DBG_ASSERT (faulty_ctx, ctx);
  return faulty_open_file (ctx, ctx->delegate.table->open_file_w, dest, fname, e);
}

/// Close always releases the file - then maybe reports a fault, like a real
/// close() that returns EIO after the fd is already gone. Failing before the
/// close would leak the delegate file, since callers don't retry close.
static err_t
faulty_close_file (void *os_self, void *file, error *e)
{
  struct faulty_ctx  *ctx = os_self;
  struct faulty_file *f   = file;
  DBG_ASSERT (faulty_ctx, ctx);
  DBG_ASSERT (faulty_file, f);
  ASSERT (f->ctx == ctx);

  const err_t ret = f->d.table->close_file (ctx->delegate.self, f->d.self, e);
  slab_alloc_free (&ctx->file_alloc, f);

  if (ret) {
    return error_trace (e);
  }

  if (faulty_roll (ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return SUCCESS;
}

static err_t
faulty_remove_quiet (void *os_self, const char *fname, error *e)
{
  struct faulty_ctx *ctx = os_self;
  DBG_ASSERT (faulty_ctx, ctx);

  if (faulty_roll (ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return ctx->delegate.table->remove_quiet (ctx->delegate.self, fname, e);
}

static err_t
faulty_unlink (void *os_self, const char *fname, error *e)
{
  struct faulty_ctx *ctx = os_self;
  DBG_ASSERT (faulty_ctx, ctx);

  if (faulty_roll (ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return ctx->delegate.table->unlink (ctx->delegate.self, fname, e);
}

static err_t
faulty_file_exists (void *os_self, const char *fname, bool *dest, error *e)
{
  struct faulty_ctx *ctx = os_self;
  DBG_ASSERT (faulty_ctx, ctx);

  if (faulty_roll (ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return ctx->delegate.table->file_exists (ctx->delegate.self, fname, dest, e);
}

////////////////////////////////////////////////////////////
// File operations

static err_t
faulty_fsync (void *self, error *e)
{
  struct faulty_file *f = self;
  DBG_ASSERT (faulty_file, f);

  if (faulty_roll (f->ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return f->d.table->fsync (f->d.self, e);
}

static i64
faulty_file_size (void *self, error *e)
{
  struct faulty_file *f = self;
  DBG_ASSERT (faulty_file, f);

  if (faulty_roll (f->ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return f->d.table->file_size (f->d.self, e);
}

static i64
faulty_read_all (void *self, void *dest, const u64 nbytes, error *e)
{
  struct faulty_file *f = self;
  DBG_ASSERT (faulty_file, f);

  if (faulty_roll (f->ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return f->d.table->read_all (f->d.self, dest, nbytes, e);
}

static i64
faulty_pread_all (void *self, void *dest, const u64 n, const u64 offset, error *e)
{
  struct faulty_file *f = self;
  DBG_ASSERT (faulty_file, f);

  if (faulty_roll (f->ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return f->d.table->pread_all (f->d.self, dest, n, offset, e);
}

static err_t
faulty_write_all (void *self, const void *src, const u64 nbytes, error *e)
{
  struct faulty_file *f = self;
  DBG_ASSERT (faulty_file, f);

  if (faulty_roll (f->ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return f->d.table->write_all (f->d.self, src, nbytes, e);
}

static err_t
faulty_pwrite_all (void *self, const void *src, const u64 n, const u64 offset, error *e)
{
  struct faulty_file *f = self;
  DBG_ASSERT (faulty_file, f);

  if (faulty_roll (f->ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return f->d.table->pwrite_all (f->d.self, src, n, offset, e);
}

static err_t
faulty_writev_all (void *self, struct bytes *iov, const int iovcnt, error *e)
{
  struct faulty_file *f = self;
  DBG_ASSERT (faulty_file, f);

  if (faulty_roll (f->ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return f->d.table->writev_all (f->d.self, iov, iovcnt, e);
}

static err_t
faulty_truncate (void *self, const u64 bytes, error *e)
{
  struct faulty_file *f = self;
  DBG_ASSERT (faulty_file, f);

  if (faulty_roll (f->ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return f->d.table->truncate (f->d.self, bytes, e);
}

static err_t
faulty_prealloc (void *self, const u64 bytes, error *e)
{
  struct faulty_file *f = self;
  DBG_ASSERT (faulty_file, f);

  if (faulty_roll (f->ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return f->d.table->prealloc (f->d.self, bytes, e);
}

static i64
faulty_seek (void *self, const u64 offset, const seek_t whence, error *e)
{
  struct faulty_file *f = self;
  DBG_ASSERT (faulty_file, f);

  if (faulty_roll (f->ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  return f->d.table->seek (f->d.self, offset, whence, e);
}

////////////////////////////////////////////////////////////
// Threads

static err_t
faulty_thread_create (
    void     *os_self,
    i_thread *dest,
    void *(*func) (void *),
    void  *context,
    error *e
)
{
  struct faulty_ctx *ctx = os_self;
  DBG_ASSERT (faulty_ctx, ctx);
  ASSERT (dest);
  ASSERT (func);

  if (faulty_roll (ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  struct faulty_thread *t = slab_alloc_alloc (&ctx->thread_alloc, e);
  if (t == NULL) {
    return error_trace (e);
  }

  if (ctx->delegate.table->thread_create (ctx->delegate.self, &t->d, func, context, e)) {
    slab_alloc_free (&ctx->thread_alloc, t);
    return error_trace (e);
  }

  t->ctx = ctx;
  DBG_ASSERT (faulty_thread, t);

  dest->table = &faulty_os_vtable;
  dest->self  = t;

  return SUCCESS;
}

static void
faulty_thread_join (void *os_self, void *thread)
{
  struct faulty_ctx    *ctx = os_self;
  struct faulty_thread *t   = thread;
  DBG_ASSERT (faulty_ctx, ctx);
  DBG_ASSERT (faulty_thread, t);
  ASSERT (t->ctx == ctx);

  t->d.table->thread_join (ctx->delegate.self, t->d.self);

  slab_alloc_free (&ctx->thread_alloc, t);
}

////////////////////////////////////////////////////////////
// Mutexes

static err_t
faulty_mutex_create (void *os_self, i_mutex *dest, error *e)
{
  struct faulty_ctx *ctx = os_self;
  DBG_ASSERT (faulty_ctx, ctx);
  ASSERT (dest);

  if (faulty_roll (ctx, e, ERR_NOMEM)) {
    return error_trace (e);
  }

  struct faulty_mutex *m = slab_alloc_alloc (&ctx->mutex_alloc, e);
  if (m == NULL) {
    return error_trace (e);
  }

  if (ctx->delegate.table->mutex_create (ctx->delegate.self, &m->d, e)) {
    slab_alloc_free (&ctx->mutex_alloc, m);
    return error_trace (e);
  }

  m->ctx = ctx;
  DBG_ASSERT (faulty_mutex, m);

  dest->table = &faulty_os_vtable;
  dest->self  = m;

  return SUCCESS;
}

static void
faulty_mutex_free (void *os_self, void *mutex)
{
  struct faulty_ctx   *ctx = os_self;
  struct faulty_mutex *m   = mutex;
  DBG_ASSERT (faulty_ctx, ctx);
  DBG_ASSERT (faulty_mutex, m);
  ASSERT (m->ctx == ctx);

  m->d.table->mutex_free (ctx->delegate.self, m->d.self);
  slab_alloc_free (&ctx->mutex_alloc, m);
}

static void
faulty_mutex_lock (void *mutex)
{
  struct faulty_mutex *m = mutex;
  DBG_ASSERT (faulty_mutex, m);

  m->d.table->mutex_lock (m->d.self);
}

static void
faulty_mutex_unlock (void *mutex)
{
  struct faulty_mutex *m = mutex;
  DBG_ASSERT (faulty_mutex, m);

  m->d.table->mutex_unlock (m->d.self);
}

////////////////////////////////////////////////////////////
// Condition variables

static err_t
faulty_cond_create (void *os_self, i_cond *dest, error *e)
{
  struct faulty_ctx *ctx = os_self;
  DBG_ASSERT (faulty_ctx, ctx);
  ASSERT (dest);

  if (faulty_roll (ctx, e, ERR_NOMEM)) {
    return error_trace (e);
  }

  struct faulty_cond *c = slab_alloc_alloc (&ctx->cond_alloc, e);
  if (c == NULL) {
    return error_trace (e);
  }

  if (ctx->delegate.table->cond_create (ctx->delegate.self, &c->d, e)) {
    slab_alloc_free (&ctx->cond_alloc, c);
    return error_trace (e);
  }

  c->ctx = ctx;
  DBG_ASSERT (faulty_cond, c);

  dest->table = &faulty_os_vtable;
  dest->self  = c;

  return SUCCESS;
}

static void
faulty_cond_free (void *os_self, void *cond)
{
  struct faulty_ctx  *ctx = os_self;
  struct faulty_cond *c   = cond;
  DBG_ASSERT (faulty_ctx, ctx);
  DBG_ASSERT (faulty_cond, c);
  ASSERT (c->ctx == ctx);

  c->d.table->cond_free (ctx->delegate.self, c->d.self);
  slab_alloc_free (&ctx->cond_alloc, c);
}

/// The mutex is a faulty wrapper too - unwrap both before delegating
static void
faulty_cond_wait (void *cond, void *mutex)
{
  struct faulty_cond  *c = cond;
  struct faulty_mutex *m = mutex;
  DBG_ASSERT (faulty_cond, c);
  DBG_ASSERT (faulty_mutex, m);
  ASSERT (c->ctx == m->ctx);

  c->d.table->cond_wait (c->d.self, m->d.self);
}

static void
faulty_cond_timed_wait (void *cond, void *mutex, u64 msec)
{
  struct faulty_cond  *c = cond;
  struct faulty_mutex *m = mutex;
  DBG_ASSERT (faulty_cond, c);
  DBG_ASSERT (faulty_mutex, m);
  ASSERT (c->ctx == m->ctx);

  c->d.table->cond_timed_wait (c->d.self, m->d.self, msec);
}

static void
faulty_cond_signal (void *cond)
{
  struct faulty_cond *c = cond;
  DBG_ASSERT (faulty_cond, c);

  c->d.table->cond_signal (c->d.self);
}

static void
faulty_cond_broadcast (void *cond)
{
  struct faulty_cond *c = cond;
  DBG_ASSERT (faulty_cond, c);

  c->d.table->cond_broadcast (c->d.self);
}

////////////////////////////////////////////////////////////
// Timers

static err_t
faulty_timer_create (void *os_self, i_timer *dest, error *e)
{
  struct faulty_ctx *ctx = os_self;
  DBG_ASSERT (faulty_ctx, ctx);
  ASSERT (dest);

  if (faulty_roll (ctx, e, ERR_IO)) {
    return error_trace (e);
  }

  struct faulty_timer *t = slab_alloc_alloc (&ctx->timer_alloc, e);
  if (t == NULL) {
    return error_trace (e);
  }

  if (ctx->delegate.table->timer_create (ctx->delegate.self, &t->d, e)) {
    slab_alloc_free (&ctx->timer_alloc, t);
    return error_trace (e);
  }

  t->ctx = ctx;
  DBG_ASSERT (faulty_timer, t);

  dest->table = &faulty_os_vtable;
  dest->self  = t;

  return SUCCESS;
}

static void
faulty_timer_free (void *os_self, void *timer)
{
  struct faulty_ctx   *ctx = os_self;
  struct faulty_timer *t   = timer;
  DBG_ASSERT (faulty_ctx, ctx);
  DBG_ASSERT (faulty_timer, t);
  ASSERT (t->ctx == ctx);

  t->d.table->timer_free (ctx->delegate.self, t->d.self);
  slab_alloc_free (&ctx->timer_alloc, t);
}

static u64
faulty_timer_now_ns (void *timer)
{
  struct faulty_timer *t = timer;
  DBG_ASSERT (faulty_timer, t);

  return t->d.table->timer_now_ns (t->d.self);
}

////////////////////////////////////////////////////////////
// Vtable

static const struct os_vtable faulty_os_vtable = {
    // Opening and closing files
    .open_file_rw    = faulty_open_file_rw,
    .open_file_r     = faulty_open_file_r,
    .open_file_w     = faulty_open_file_w,
    .close_file      = faulty_close_file,

    // File system
    .remove_quiet    = faulty_remove_quiet,
    .unlink          = faulty_unlink,
    .file_exists     = faulty_file_exists,

    // File operations
    .fsync           = faulty_fsync,
    .file_size       = faulty_file_size,
    .read_all        = faulty_read_all,
    .pread_all       = faulty_pread_all,
    .write_all       = faulty_write_all,
    .pwrite_all      = faulty_pwrite_all,
    .writev_all      = faulty_writev_all,
    .truncate        = faulty_truncate,
    .prealloc        = faulty_prealloc,
    .seek            = faulty_seek,

    // Threads
    .thread_create   = faulty_thread_create,
    .thread_join     = faulty_thread_join,

    // Mutexes
    .mutex_create    = faulty_mutex_create,
    .mutex_free      = faulty_mutex_free,
    .mutex_lock      = faulty_mutex_lock,
    .mutex_unlock    = faulty_mutex_unlock,

    // Condition variables
    .cond_create     = faulty_cond_create,
    .cond_free       = faulty_cond_free,
    .cond_wait       = faulty_cond_wait,
    .cond_timed_wait = faulty_cond_timed_wait,
    .cond_signal     = faulty_cond_signal,
    .cond_broadcast  = faulty_cond_broadcast,

    // Timer
    .timer_create    = faulty_timer_create,
    .timer_free      = faulty_timer_free,
    .timer_now_ns    = faulty_timer_now_ns,
};

////////////////////////////////////////////////////////////
// Lifecycle

err_t
faulty_os_create (
    struct i_mem mem,
    struct i_os  delegate,
    float        fail_percent,
    struct i_os *dest,
    error       *e
)
{
  ASSERT (dest);
  ASSERT (mem.table);
  ASSERT (delegate.table);
  ASSERT (fail_percent >= 0.0f && fail_percent <= 1.0f);

  struct faulty_ctx *ctx = mem.table->calloc (mem.self, 1, sizeof *ctx, e);
  if (ctx == NULL) {
    return error_trace (e);
  }

  ctx->delegate     = delegate;
  ctx->mem          = mem;
  ctx->fail_percent = fail_percent;

  slab_alloc_init (&ctx->file_alloc, mem, sizeof (struct faulty_file), 10);
  slab_alloc_init (&ctx->thread_alloc, mem, sizeof (struct faulty_thread), 10);
  slab_alloc_init (&ctx->mutex_alloc, mem, sizeof (struct faulty_mutex), 10);
  slab_alloc_init (&ctx->cond_alloc, mem, sizeof (struct faulty_cond), 10);
  slab_alloc_init (&ctx->timer_alloc, mem, sizeof (struct faulty_timer), 10);

  DBG_ASSERT (faulty_ctx, ctx);

  dest->table = &faulty_os_vtable;
  dest->self  = ctx;

  return SUCCESS;
}

/// Frees the faulty layer - the delegate OS is borrowed and stays alive.
void
faulty_os_free (struct i_os os)
{
  ASSERT (os.table == &faulty_os_vtable);

  struct faulty_ctx *ctx = os.self;
  DBG_ASSERT (faulty_ctx, ctx);

  slab_alloc_destroy (&ctx->file_alloc);
  slab_alloc_destroy (&ctx->thread_alloc);
  slab_alloc_destroy (&ctx->mutex_alloc);
  slab_alloc_destroy (&ctx->cond_alloc);
  slab_alloc_destroy (&ctx->timer_alloc);

  // Copy out before freeing - mem lives inside ctx
  struct i_mem mem = ctx->mem;
  mem.table->free (mem.self, ctx);
}
