Use Smart Files
===============

A smart file is the same storage engine without the type system or variables:
one untyped byte array in a file, with the same ACID guarantees. Reach for it
when you want transactional inserts into the middle of a blob and don't need a
schema.

Open and use a smart file
-------------------------

Open it with `ns_smfile_open` and address it with byte offsets rather than
queries. `size` is the width of one element and `stride` is how many elements to
step between touches, so a strided write spreads data out without a query
language:

```c
#include "numstore.h"

#include <inttypes.h>
#include <stdio.h>

static void
dump (nsdb_t *smf, const char *label)
{
  uint32_t out[8] = {0};
  sb_size  n = ns_smfile_read (smf, NULL, out, sizeof (uint32_t), 0, 1, 8);
  printf ("%-16s (%" PRId64 "):", label, n);
  for (sb_size i = 0; i < n; i++) {
    printf (" %u", out[i]);
  }
  printf ("\n");
}

int
main (void)
{
  ns_cleanup ("stream.smf");

  nsdb_t *smf = ns_smfile_open ("stream.smf");
  if (smf == NULL) {
    fprintf (stderr, "could not open stream.smf\n");
    return 1;
  }

  // A smart file is one untyped byte array. Offsets and lengths are bytes.
  const uint32_t src[] = {1, 2, 3, 4, 5, 6};
  sb_size        n = ns_smfile_insert (smf, NULL, src, 0, sizeof (src));
  printf (
      "inserted %" PRId64 " bytes, file is %" PRId64 " bytes\n",
      n,
      ns_smfile_size (smf, NULL)
  );
  dump (smf, "after insert");

  // Overwrite every other u32 starting at byte 0.
  const uint32_t patch[] = {70, 80, 90};
  ns_smfile_write (smf, NULL, patch, sizeof (uint32_t), 0, 2, 3);
  dump (smf, "after write");

  // Read every other u32.
  uint32_t strided[3] = {0};
  ns_smfile_read (smf, NULL, strided, sizeof (uint32_t), 0, 2, 3);
  printf ("strided read   : %u %u %u\n", strided[0], strided[1], strided[2]);

  // Remove the first two u32s and keep what came out.
  uint32_t taken[2] = {0};
  ns_smfile_remove (smf, NULL, taken, sizeof (uint32_t), 0, 1, 2);
  printf ("removed        : %u %u\n", taken[0], taken[1]);
  dump (smf, "after remove");

  return ns_close (smf);
}
```

```text
inserted 24 bytes, file is 24 bytes
after insert     (6): 1 2 3 4 5 6
after write      (6): 70 2 80 4 90 6
strided read   : 70 80 90
removed        : 70 2
after remove     (4): 80 4 90 6
```

What smart files share with databases
-------------------------------------

Smart files share the rest of the API: `ns_begin`, `ns_commit`, `ns_rollback`,
`ns_close`, `ns_crash` and `ns_cleanup` all work on an `nsdb_t *` from
`ns_smfile_open`. What they don't share is the query language - a smart file has
no variables, so `ns_execute`, `ns_read`, `ns_write` and plans don't apply.

Keep reads in the buffer
------------------------

One sharp edge: unlike `ns_read`, the smart file functions take **no destination
length**. `nelem` is the only thing bounding the write into `dest`, so it is on
you to keep `size * nelem` within your buffer:

```c
uint32_t out[8];

// Safe: nelem matches the buffer.
ns_smfile_read (smf, NULL, out, sizeof (uint32_t), 0, 1, 8);

// Overflows out if the file holds more than 8 elements - NS_END means
// "to the end of the file", and the call does not know how big out is.
ns_smfile_read (smf, NULL, out, sizeof (uint32_t), 0, 1, NS_END);
```

Use `NS_END` only when the buffer can hold the whole file, which you can check
first with `ns_smfile_size`. Otherwise pass an explicit `nelem` and loop.
