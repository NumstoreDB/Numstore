#include "core/ns_json.h"

#include <stdio.h>

void
print_json_start (void)
{
  fputs ("{", stdout);
}

void
print_entry (const char *key, const char *value)
{
  printf ("\"%s\":%s,", key, value);
}

void
print_last_entry (const char *key, const char *value)
{
  printf ("\"%s\":%s", key, value);
}

void
print_json_end (void)
{
  fputs ("}\n", stdout);
}
