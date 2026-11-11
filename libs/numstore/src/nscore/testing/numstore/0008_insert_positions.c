
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

#include "core/ns_stdtypes.h"
#include "core/ns_utils.h"
#include "numstore.h"

#include <string.h>

#ifndef NDEBUG
#  include "core/ns_testing.h"
#endif

#ifndef NDEBUG

TEST (ns_insert_positions)
{
  const u32 base[5] = {0, 1, 2, 3, 4};
  const u32 src[3]  = {100, 101, 102};

  const struct
  {
    const char *query;
    b_size      nsrc; // Elements handed to ns_write
    sb_size     ninserted;
    u32         expected[8];
  } cases[] = {
      {"insert foo 0 3", 3, 3, {100, 101, 102, 0, 1, 2, 3, 4}},
      {"insert foo 2 3", 3, 3, {0, 1, 100, 101, 102, 2, 3, 4}},
      {"insert foo 5 3", 3, 3, {0, 1, 2, 3, 4, 100, 101, 102}},
      {"insert foo 4 1", 3, 1, {0, 1, 2, 3, 100, 4}},

      // A negative offset counts from the end
      {"insert foo -1 2", 3, 2, {0, 1, 2, 3, 100, 101, 4}},
      {"insert foo -5 1", 3, 1, {100, 0, 1, 2, 3, 4}},

      // An insert stops at the end of the source
      {"insert foo 1 3", 2, 2, {0, 100, 101, 1, 2, 3, 4}},
  };

  for (u32 i = 0; i < arrlen (cases); ++i) {
    TEST_CASE ("%s with %d elements", cases[i].query, (int)cases[i].nsrc)
    {
      test_assert_int_equal (ns_cleanup ("test"), 0);
      nsdb_t *db = ns_open ("test");
      test_assert (db != NULL);

      test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);
      test_assert_int_equal (ns_write (db, NULL, base, sizeof base, "insert foo 0 5"), 5);

      sb_size
          ninserted = ns_write (db, NULL, src, cases[i].nsrc * sizeof (u32), "%s", cases[i].query);
      test_assert_int_equal (ninserted, cases[i].ninserted);

      u32     dest[9];
      sb_size len = 5 + cases[i].ninserted;
      test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), len);
      test_assert_memequal (dest, cases[i].expected, (size_t)len * sizeof (u32));

      test_assert_int_equal (ns_close (db), 0);
    }
  }

  TEST_CASE ("Inserts into the middle of a variable that spans many pages")
  {
    enum
    {
      LEN   = 20000,
      CHUNK = 7,
    };

    static u32 model[LEN + 100 * CHUNK];
    static u32 dest[LEN + 100 * CHUNK];

    test_assert_int_equal (ns_cleanup ("test"), 0);
    nsdb_t *db = ns_open ("test");
    test_assert (db != NULL);

    for (u32 i = 0; i < LEN; ++i) {
      model[i] = i;
    }
    test_assert_int_equal (ns_execute (db, NULL, "create foo u32"), 0);
    test_assert_int_equal (
        ns_write (db, NULL, model, LEN * sizeof (u32), "insert foo 0 %d", LEN),
        LEN
    );

    // 100 small inserts spread over the variable
    u32 len = LEN;
    for (u32 i = 0; i < 100; ++i) {
      u32 chunk[CHUNK];
      for (u32 k = 0; k < CHUNK; ++k) {
        chunk[k] = 1000000 + i * CHUNK + k;
      }

      u32 ofst = (i * 7919) % len;
      test_assert_int_equal (
          ns_write (db, NULL, chunk, sizeof chunk, "insert foo %u %d", ofst, CHUNK),
          CHUNK
      );

      memmove (&model[ofst + CHUNK], &model[ofst], (len - ofst) * sizeof (u32));
      memcpy (&model[ofst], chunk, sizeof chunk);
      len += CHUNK;
    }

    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), len);
    test_assert_memequal (dest, model, len * sizeof (u32));

    // Still there after a reopen
    test_assert_int_equal (ns_close (db), 0);
    db = ns_open ("test");
    test_assert (db != NULL);

    test_assert_int_equal (ns_read (db, NULL, dest, sizeof dest, "read foo"), len);
    test_assert_memequal (dest, model, len * sizeof (u32));

    test_assert_int_equal (ns_close (db), 0);
  }
}

#endif
