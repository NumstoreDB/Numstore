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

#include "c_specx.h"
#include "numstore/types.h"

static void print_indent (int level, u32 spaces) {
  for (u32 i = 0; i < spaces; ++i) { i_printf (level, " "); }
}

static void print_prim_value (int level, const u8 *buf, enum prim_t p) {
  switch (p) {
    case U8: {
      u8 v;
      memcpy (&v, buf, 1);
      i_printf (level, "%u", (unsigned)v);
      return;
    }
    case U16: {
      u16 v;
      memcpy (&v, buf, 2);
      i_printf (level, "%u", (unsigned)v);
      return;
    }
    case U32: {
      u32 v;
      memcpy (&v, buf, 4);
      i_printf (level, "%u", (unsigned)v);
      return;
    }
    case U64: {
      u64 v;
      memcpy (&v, buf, 8);
      i_printf (level, "%lu", (unsigned long)v);
      return;
    }
    case I8: {
      i8 v;
      memcpy (&v, buf, 1);
      i_printf (level, "%d", (int)v);
      return;
    }
    case I16: {
      i16 v;
      memcpy (&v, buf, 2);
      i_printf (level, "%d", (int)v);
      return;
    }
    case I32: {
      i32 v;
      memcpy (&v, buf, 4);
      i_printf (level, "%d", (int)v);
      return;
    }
    case I64: {
      i64 v;
      memcpy (&v, buf, 8);
      i_printf (level, "%ld", (long)v);
      return;
    }
    case F16: {
      u16 h;
      memcpy (&h, buf, 2);
      i_printf (level, "%g", (double)f16_to_f32 (h));
      return;
    }
    case F32: {
      float v;
      memcpy (&v, buf, 4);
      i_printf (level, "%g", (double)v);
      return;
    }
    case F64: {
      double v;
      memcpy (&v, buf, 8);
      i_printf (level, "%g", v);
      return;
    }
    case F128: {
      u64 lo, hi;
      memcpy (&lo, buf, 8);
      memcpy (&hi, buf + 8, 8);
      i_printf (level, "<f128:0x%016lx%016lx>", (unsigned long)hi, (unsigned long)lo);
      return;
    }
    case CF32: {
      u16 rh, ih;
      memcpy (&rh, buf, 2);
      memcpy (&ih, buf + 2, 2);
      i_printf (level, "(%g, %g)", (double)f16_to_f32 (rh), (double)f16_to_f32 (ih));
      return;
    }
    case CF64: {
      float r, im;
      memcpy (&r, buf, 4);
      memcpy (&im, buf + 4, 4);
      i_printf (level, "(%g, %g)", (double)r, (double)im);
      return;
    }
    case CF128: {
      double r, im;
      memcpy (&r, buf, 8);
      memcpy (&im, buf + 8, 8);
      i_printf (level, "(%g, %g)", r, im);
      return;
    }
    case CF256: {
#if SIZEOF_LONG_DOUBLE >= 16
      long double r, im;
      memcpy (&r, buf, 16);
      memcpy (&im, buf + 16, 16);
      i_printf (level, "(%Lg, %Lg)", r, im);
#else
      u64 r_lo, r_hi, im_lo, im_hi;
      memcpy (&r_lo, buf, 8);
      memcpy (&r_hi, buf + 8, 8);
      memcpy (&im_lo, buf + 16, 8);
      memcpy (&im_hi, buf + 24, 8);
      i_printf (
          level,
          "(<f128:0x%016lx%016lx>, "
          "<f128:0x%016lx%016lx>)",
          (unsigned long)r_hi,
          (unsigned long)r_lo,
          (unsigned long)im_hi,
          (unsigned long)im_lo);
#endif
      return;
    }
    case CI16: {
      i8 r, im;
      memcpy (&r, buf, 1);
      memcpy (&im, buf + 1, 1);
      i_printf (level, "(%d, %d)", (int)r, (int)im);
      return;
    }
    case CI32: {
      i16 r, im;
      memcpy (&r, buf, 2);
      memcpy (&im, buf + 2, 2);
      i_printf (level, "(%d, %d)", (int)r, (int)im);
      return;
    }
    case CI64: {
      i32 r, im;
      memcpy (&r, buf, 4);
      memcpy (&im, buf + 4, 4);
      i_printf (level, "(%d, %d)", (int)r, (int)im);
      return;
    }
    case CI128: {
      i64 r, im;
      memcpy (&r, buf, 8);
      memcpy (&im, buf + 8, 8);
      i_printf (level, "(%ld, %ld)", (long)r, (long)im);
      return;
    }
    case CU16: {
      u8 r, im;
      memcpy (&r, buf, 1);
      memcpy (&im, buf + 1, 1);
      i_printf (level, "(%u, %u)", (unsigned)r, (unsigned)im);
      return;
    }
    case CU32: {
      u16 r, im;
      memcpy (&r, buf, 2);
      memcpy (&im, buf + 2, 2);
      i_printf (level, "(%u, %u)", (unsigned)r, (unsigned)im);
      return;
    }
    case CU64: {
      u32 r, im;
      memcpy (&r, buf, 4);
      memcpy (&im, buf + 4, 4);
      i_printf (level, "(%u, %u)", (unsigned)r, (unsigned)im);
      return;
    }
    case CU128: {
      u64 r, im;
      memcpy (&r, buf, 8);
      memcpy (&im, buf + 8, 8);
      i_printf (level, "(%lu, %lu)", (unsigned long)r, (unsigned long)im);
      return;
    }
  }
}

static void
print_type_inner (int level, const u8 *buf, const struct type *t, u32 max_elems, u32 indent);

// Product of dims[dim_idx+1 .. rank-1] * element_size
static u32 sarray_sub_size (const struct sarray_t *sa, u16 dim_idx) {
  u32 sub = type_byte_size (sa->t);
  for (u16 i = dim_idx + 1; i < sa->rank; ++i) { sub *= sa->dims[i]; }
  return sub;
}

// col: visual column of the '[' just printed at this dimension,
// used to align continuation rows under it.
static void print_sarray_dim (
    int                    level,
    const u8              *buf,
    const struct sarray_t *sa,
    u16                    dim_idx,
    u32                    max_elems,
    u32                    indent,
    u32                    col) {
  u32 dim_len  = sa->dims[dim_idx];
  u32 show     = dim_len < max_elems ? dim_len : max_elems;
  u32 sub_size = sarray_sub_size (sa, dim_idx);

  i_printf (level, "[");

  if (dim_idx == sa->rank - 1) {
    // Innermost dimension: elements inline
    for (u32 i = 0; i < show; ++i) {
      if (i > 0) { i_printf (level, ", "); }
      print_type_inner (level, buf + i * sub_size, sa->t, max_elems, indent + 1);
    }
    if (dim_len > max_elems) { i_printf (level, ", ..."); }
  } else {
    // Outer dimension: each sub-array on its own line
    for (u32 i = 0; i < show; ++i) {
      if (i > 0) {
        i_printf (level, ",\n");
        print_indent (level, col + 1);
      }
      print_sarray_dim (level, buf + i * sub_size, sa, dim_idx + 1, max_elems, indent + 1, col + 1);
    }
    if (dim_len > max_elems) {
      i_printf (level, ",\n");
      print_indent (level, col + 1);
      i_printf (level, "...");
    }
  }

  i_printf (level, "]");
}

static void
print_type_inner (int level, const u8 *buf, const struct type *t, u32 max_elems, u32 indent) {
  switch (t->type) {
    case T_PRIM: {
      print_prim_value (level, buf, t->p);
      return;
    }

    case T_STRUCT: {
      i_printf (level, "{\n");
      u32 offset = 0;
      for (u16 i = 0; i < t->st.len; ++i) {
        u32 field_indent = indent + 4;
        print_indent (level, field_indent);
        i_printf (level, "%.*s = ", (int)t->st.keys[i].len, t->st.keys[i].data);

        const struct type *ft = t->st.types[i];
        if (ft->type == T_SARRAY) {
          u32 col = field_indent + t->st.keys[i].len + 3;
          print_sarray_dim (level, buf + offset, &ft->sa, 0, max_elems, field_indent, col);
        } else {
          print_type_inner (level, buf + offset, ft, max_elems, field_indent);
        }

        offset += type_byte_size (ft);
        if (i + 1 < t->st.len) { i_printf (level, ","); }
        i_printf (level, "\n");
      }
      print_indent (level, indent);
      i_printf (level, "}");
      return;
    }

    case T_UNION: {
      i_printf (level, "<union[0]: ");
      if (t->un.len > 0) {
        i_printf (level, "%.*s = ", (int)t->un.keys[0].len, t->un.keys[0].data);
        print_type_inner (level, buf, t->un.types[0], max_elems, indent);
      } else {
        i_printf (level, "empty");
      }
      i_printf (level, ">");
      return;
    }

    case T_SARRAY: {
      print_sarray_dim (level, buf, &t->sa, 0, max_elems, indent, indent);
      return;
    }
  }
}

void type_print_data (int log_level, const u8 *buf, const struct type *t, u32 max_elems) {
  print_type_inner (log_level, buf, t, max_elems, 0);
  i_printf (log_level, "\n");
}

struct type_printer_ostream_ctx {
  struct type *t;
  t_size       pos;
  t_size       size;
  u8           buf[];
};

static i32
type_print_os_sink (struct stream *s, void *vctx, const void *src, u32 size, u32 n, error *e) {
  ASSERT (size == 1);
  struct type_printer_ostream_ctx *ctx = (struct type_printer_ostream_ctx *)vctx;

  u32 avail = ctx->size - ctx->pos;
  u32 next  = MIN (avail, n);

  if (next == 0) { return 0; }

  memcpy (ctx->buf + ctx->pos, src, next);
  ctx->pos += next;

  if (ctx->pos == ctx->size) {
    type_print_data (LOG_INFO, ctx->buf, ctx->t, 3);
    ctx->pos = 0;
  }

  return (i32)next;
}

static void type_print_os_close (void *ctx) { i_free ((struct type_printer_ostream_ctx *)ctx); }

static const struct stream_ops type_printer_os_ops = {
    .pull  = NULL,
    .push  = type_print_os_sink,
    .close = type_print_os_close,
};

err_t type_stream_printer_init (struct stream *s, struct type *t, error *e) {
  t_size                           size = type_byte_size (t);
  struct type_printer_ostream_ctx *ctx  = i_malloc (1, sizeof *ctx + size, e);
  if (ctx == NULL) { return error_trace (e); }

  ctx->size = size;
  ctx->pos  = 0;
  ctx->t    = t;
  stream_init (s, &type_printer_os_ops, ctx);

  return SUCCESS;
}
