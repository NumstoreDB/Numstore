#include "core/ns_logger.h"

void
console_flush (struct console *c)
{
  c->out.table->fsync (c->out.self, &c->e);
}

static inline err_t
i_vfprintf (struct i_file *f, char *buffer, u32 blen, const char *fmt, error *e, va_list args)
{
  // IGNORE ERROR
  int n = vsnprintf (buffer, blen, fmt, args);
  return f->table->write_all (f->self, buffer, n, e);
}

static inline err_t
i_fprintf (struct i_file *f, char *buffer, u32 blen, const char *fmt, error *e, ...)
{
  va_list args;
  va_start (args, e);
  err_t ret = i_vfprintf (f, buffer, blen, fmt, e, args);
  va_end (args);
  return ret;
}

//////////////////////// PRINTING
void
console_printf (struct console *c, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  // IGNORE ERROR
  i_vfprintf (&c->out, c->buffer, sizeof (c->buffer), fmt, &c->e, args);
  va_end (args);
}

void
console_printf_err (struct console *c, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  // IGNORE ERROR
  i_vfprintf (&c->err, c->buffer, sizeof (c->buffer), fmt, &c->e, args);
  va_end (args);
}

//////////////////////// LOGGING
void
console_logf (struct console *c, const char *prefix, const char *color, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  // IGNORE ERROR
  i_fprintf (&c->out, c->buffer, sizeof (c->buffer), "%s[%-8.8s]: ", &c->e, color, prefix);
  i_vfprintf (&c->out, c->buffer, sizeof (c->buffer), fmt, &c->e, args);
  i_fprintf (&c->out, c->buffer, sizeof (c->buffer), "%s", &c->e, RESET);
  va_end (args);
}

void
console_logf_err (struct console *c, const char *prefix, const char *color, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  // IGNORE ERROR
  i_fprintf (&c->err, c->buffer, sizeof (c->buffer), "%s[%-8.8s]: ", &c->e, color, prefix);
  i_vfprintf (&c->err, c->buffer, sizeof (c->buffer), fmt, &c->e, args);
  i_fprintf (&c->err, c->buffer, sizeof (c->buffer), "%s", &c->e, RESET);
  va_end (args);
}
