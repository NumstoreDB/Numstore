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

#include "numstore.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct example
{
  union {
    int32_t  a;
    uint64_t b;
    uint64_t c;
  } d;
  float   e[20][256];
  int32_t f;
} __attribute__ ((packed));

/**
 * This example shows basic first class operations of smart files
 * Namely:
 *
 * 1. Insert (insert data into the middle of an array)
 * 2. Read (normal read)
 * 3. Write (overwrite data in the middle of the array)
 * 4. Remove (remove chunks of data from the middle of an array)
 */
int
main (void)
{
  // Open a new data file
  nsdb_cleanup ("sample1_crud");
  nsdb_t *ns = nsdb_open ("sample1_crud");
  if (ns == NULL) { return -1; }

  // Insert out initial sentence
  nsdb_create (ns, "example", "struct { d union { a u32, b u64, c u64 }, e [20][256]f32, f i32 }");

  int             len  = 200;
  struct example *src  = malloc (sizeof (struct example) * len);
  struct example *dest = malloc (sizeof (struct example) * len);

  nsdb_var_t *var = nsdb_get (ns, "example");
  nsdb_insert (ns, var, src, 0, len);

  int     flags = START_PRESENT | STOP_PRESENT | STEP_PRESENT;
  sb_size n     = nsdb_read (ns, var, dest, 0, 1, -1, flags);

  flags = START_PRESENT | STOP_PRESENT | STEP_PRESENT;
  n     = nsdb_remove (ns, var, dest, 0, 1, -10, flags);

  flags = START_PRESENT | STOP_PRESENT | STEP_PRESENT;
  n     = nsdb_read (ns, var, dest, 0, 1, -1, flags);

  flags = START_PRESENT | STOP_PRESENT | STEP_PRESENT;
  n     = nsdb_write (ns, var, src, 0, 1, -1, flags);

  flags = START_PRESENT | STOP_PRESENT | STEP_PRESENT;
  n     = nsdb_read (ns, var, dest, 0, 1, -1, flags);

  nsdb_free (var);

  free (src);
  free (dest);

  return nsdb_close (ns);
}
