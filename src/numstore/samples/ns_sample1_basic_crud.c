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
  float    a;
  int32_t  b;
  uint32_t d[5][10];
} __attribute__ ((packed));

// Little utils
static void print_example (const char *label, struct example *ex, int size);
static void init_example (struct example *ex, int size);

// Our source and destination buffers
static struct example src[200];
static struct example dest[200];

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
  numstore_cleanup ("sample1_crud");
  numstore_t *ns = numstore_open ("sample1_crud");
  if (ns == NULL) {
    return -1;
  }

  // Create a new variable (no data buffer involved)
  struct numstore_plan create_plan = {0};
  numstore_fexecute (
      ns,
      NULL,
      &create_plan,
      "create example struct {\n"
      "  a f32,\n"
      "  b i32,\n"
      "  d [5][10] u32\n"
      "}"
  );

  init_example (src, 200);

  // Insert data at offset 0
  struct numstore_plan insert_plan = {.data = src, .dlen = sizeof (src)};
  sb_size              n = numstore_fexecute (ns, NULL, &insert_plan, "insert example 0 %d", 200);

  // Read (most of) data with a stride of 3
  struct numstore_plan read_plan = {.data = dest, .dlen = sizeof (dest)};
  n = numstore_fexecute (ns, NULL, &read_plan, "read example[0:-10:3] blimit %ld", sizeof (dest));
  print_example ("Read elements: ", dest, n);

  // Remove (most of) data with a stride of 2
  struct numstore_plan remove_plan = {.data = dest, .dlen = sizeof (dest)};
  n                                = numstore_fexecute (
      ns,
      NULL,
      &remove_plan,
      "remove example[0:-10:2] blimit %ld",
      sizeof (dest)
  );
  print_example ("Removed elements: ", dest, n);

  // Read all of data
  n = numstore_fexecute (ns, NULL, &read_plan, "read example[0:] blimit %ld", sizeof (dest));
  print_example ("After Remove: ", dest, n);

  // Write all of data with src
  struct numstore_plan write_plan = {.data = src, .dlen = sizeof (src)};
  n = numstore_fexecute (ns, NULL, &write_plan, "write example[0::] blimit %ld", sizeof (src));
  n = numstore_fexecute (ns, NULL, &read_plan, "read example[0:] blimit %ld", sizeof (dest));
  print_example ("After write: ", dest, n);

  return numstore_close (ns);
}

static void
print_example (const char *label, struct example *ex, int size)
{
  int show = size > 10 ? 10 : size;

  printf ("%s: examples (%d):\n", label, size);
  for (int i = 0; i < show; i++) {
    printf (
        "%s:   [%d] a=%g  b=%d  d=[[%u, %u ...], [%u, %u ...], "
        "...]\n",
        label,
        i,
        ex[i].a,
        ex[i].b,
        ex[i].d[0][0],
        ex[i].d[0][1],
        ex[i].d[1][0],
        ex[i].d[1][1]
    );
  }
  if (size > show) {
    printf ("%s:   ... (%d more)\n", label, size - show);
  }
}

static void
init_example (struct example *ex, int size)
{
  for (int i = 0; i < size; i++) {
    ex[i].a = i;
    ex[i].b = i + 1;
    for (int r = 0; r < 5; r++) {
      for (int c = 0; c < 10; c++) {
        ex[i].d[r][c] = i + r * 10 + c;
      }
    }
  }
}
