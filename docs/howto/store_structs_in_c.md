Store Structs and Arrays from C
===============================

Numstore types have no padding. A struct is the sum of its members. A strict
array is the product of its dimensions. So a C struct that you read into must
be packed. If it is not packed, the padding of the compiler moves each field.

```c
#include "numstore.h"

#include <inttypes.h>
#include <stdio.h>

// Numstore types have no padding, so the C struct must have none either.
#ifdef _MSC_VER
#  define PACK_PUSH __pragma (pack (push, 1))
#  define PACK_POP  __pragma (pack (pop))
#  define PACKED
#else
#  define PACK_PUSH
#  define PACK_POP
#  define PACKED __attribute__ ((packed))
#endif

PACK_PUSH
struct frame
{
  uint64_t timestamp;
  uint8_t  pixels[4][4];
} PACKED;
PACK_POP

int
main (void)
{
  ns_cleanup ("frames.db");
  nsdb_t *db = ns_open ("frames.db");

  if (ns_execute (
          db, NULL, "create frames struct { timestamp u64, pixels [4][4] u8 }"
      ) != 0) {
    fprintf (stderr, "create failed: %s\n", ns_strerror (db, false));
    ns_close (db);
    return 1;
  }

  printf ("sizeof (struct frame) = %zu\n", sizeof (struct frame));

  struct frame in[2] = {0};
  for (int f = 0; f < 2; f++) {
    in[f].timestamp = 1000 + f;
    for (int r = 0; r < 4; r++) {
      for (int c = 0; c < 4; c++) {
        in[f].pixels[r][c] = (uint8_t)(f * 16 + r * 4 + c);
      }
    }
  }

  sb_size wrote = ns_write (db, NULL, in, sizeof (in), "insert frames 0 2");
  printf ("wrote %" PRId64 " frames\n", wrote);

  struct frame out[2] = {0};
  sb_size      read = ns_read (db, NULL, out, sizeof (out), "read frames[0:]");
  printf ("read %" PRId64 " frames\n", read);
  for (sb_size i = 0; i < read; i++) {
    printf (
        "frame %" PRId64 ": timestamp=%" PRIu64
        " pixels[0][0]=%u pixels[3][3]=%u\n",
        i,
        out[i].timestamp,
        out[i].pixels[0][0],
        out[i].pixels[3][3]
    );
  }

  nsdb_var_t *var = ns_get_var (db, NULL, "get frames");
  printf ("variable holds %" PRIu64 " elements\n", ns_var_len (var));
  ns_var_free (var);

  return ns_close (db);
}
```

```text
sizeof (struct frame) = 24
wrote 2 frames
read 2 frames
frame 0: timestamp=1000 pixels[0][0]=0 pixels[3][3]=15
frame 1: timestamp=1001 pixels[0][0]=16 pixels[3][3]=31
variable holds 2 elements
```

The size is `8 + 4 x 4 = 24` bytes. Numstore and the packed struct agree on
this size. Arrays are in C order. So `pixels[r][c]` matches element for
element. [The type system](../concepts/types.md) gives the size rules for
every type.
