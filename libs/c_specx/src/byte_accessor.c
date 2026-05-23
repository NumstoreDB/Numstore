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

static u32
ba_memcpy_from_recursive (u8 *dest, const u8 *src, struct byte_accessor *acc)
{
  switch (acc->type)
  {
    case TA_TAKE:
    {
      memcpy (dest, src, acc->src_size);
      return acc->src_size;
    }
    case TA_SELECT:
    {
      return ba_memcpy_from_recursive (dest, src + acc->select.bofst, acc->select.sub_ba);
    }
    case TA_RANGE:
    {
      u32 elem_size = acc->range.sub_ba->src_size;
      u32 pos       = acc->range.stride.start;
      u32 written   = 0;
      u32 i         = 0;

      while (i < acc->range.stride.nelems)
      {
        written +=
            ba_memcpy_from_recursive (dest + written, src + pos * elem_size, acc->range.sub_ba);

        pos += acc->range.stride.stride;
        i++;
      }

      return written;
    }
  }
  UNREACHABLE ();
}

u32
ba_memcpy_from (u8 *dest, const u8 *src, struct byte_accessor *acc)
{ return ba_memcpy_from_recursive (dest, src, acc); }

#ifndef NTEST
TEST (ba_memcpy_from_basic)
{
  // struct { a int, b struct { b char, c [5]u16 } }
  u8 src[64];
  u8 dest[64];

  u8 test_data[] = {
      78,
      56,
      34,
      12,   // int a (little-endian)
      0xAB, // char b
      1,
      0, // u16 c[0] = 1
      2,
      0, // u16 c[1] = 2
      3,
      0, // u16 c[2] = 3
      4,
      0, // u16 c[3] = 4
      5,
      0, // u16 c[4] = 5
  };

  memcpy (src, test_data, sizeof (test_data));

  TEST_CASE ("[.a]")
  {
    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 4,
        .select    = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };

    u32 moved = ba_memcpy_from (dest, src, &acc);
    test_assert_int_equal (moved, 4);

    test_assert_int_equal (dest[0], 78);
    test_assert_int_equal (dest[1], 56);
    test_assert_int_equal (dest[2], 34);
    test_assert_int_equal (dest[3], 12);
  }

  TEST_CASE ("[.b]")
  {
    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 1,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 1,
                .dest_size = 1,
            },
        },
    };

    u32 moved = ba_memcpy_from (dest, src, &acc);
    test_assert_int_equal (moved, 1);

    test_assert_int_equal (dest[0], 0xAB);
  }

  TEST_CASE ("[.b, .a]")
  {
    struct byte_accessor dotb = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 1,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 1,
                .dest_size = 1,
            },
        },
    };
    struct byte_accessor dota = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 4,
        .select    = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };

    u32 moved = ba_memcpy_from (dest, src, &dotb);
    test_assert_int_equal (moved, 1);
    moved = ba_memcpy_from (dest + moved, src, &dota);
    test_assert_int_equal (moved, 4);

    test_assert_int_equal (dest[0], 0xAB);
    test_assert_int_equal (dest[1], 78);
    test_assert_int_equal (dest[2], 56);
    test_assert_int_equal (dest[3], 34);
    test_assert_int_equal (dest[4], 12);
  }

  TEST_CASE ("[.a, .b]")
  {
    struct byte_accessor dota = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 4,
        .select    = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };
    struct byte_accessor dotb = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 1,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 1,
                .dest_size = 1,
            },
        },
    };

    u32 moved = ba_memcpy_from (dest, src, &dota);
    test_assert_int_equal (moved, 4);
    moved = ba_memcpy_from (dest + moved, src, &dotb);
    test_assert_int_equal (moved, 1);

    test_assert_int_equal (dest[0], 78);
    test_assert_int_equal (dest[1], 56);
    test_assert_int_equal (dest[2], 34);
    test_assert_int_equal (dest[3], 12);
    test_assert_int_equal (dest[4], 0xAB);
  }

  TEST_CASE ("[.b.c[1:1:4]]")
  {
    // SELECT(4, SELECT(1, RANGE(start=1, stride=1, nelems=4,
    // TAKE(2)))) Gathers c[1], c[2], c[3] → 6 bytes
    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 6,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_SELECT,
                .src_size  = 11,
                .dest_size = 6,
                .select    = {
                    .bofst  = 1,
                    .sub_ba = &(struct byte_accessor){
                        .type      = TA_RANGE,
                        .src_size  = 10,
                        .dest_size = 8,
                        .range     = {
                            .sub_ba =
                                &(struct byte_accessor){
                                    .type      = TA_TAKE,
                                    .src_size  = 2,
                                    .dest_size = 2,
                                },
                            .stride = (struct stride){
                                .start  = 1,
                                .stride = 1,
                                .nelems = 3,
                            },
                        },
                    },
                },
            },
        },
    };

    u32 moved = ba_memcpy_from (dest, src, &acc);
    test_assert_int_equal (moved, 6);

    test_assert_int_equal (dest[0], 2); // c[1] = 2
    test_assert_int_equal (dest[1], 0);
    test_assert_int_equal (dest[2], 3); // c[2] = 3
    test_assert_int_equal (dest[3], 0);
    test_assert_int_equal (dest[4], 4); // c[3] = 4
    test_assert_int_equal (dest[5], 0);
  }

  TEST_CASE ("[.b.c[0:2:5]]")
  {
    // SELECT(4, SELECT(1, RANGE(start=0, stride=2, nelems=5,
    // TAKE(2)))) Gathers c[0], c[2], c[4] → 6 bytes
    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 15,
        .dest_size = 6,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_SELECT,
                .src_size  = 11,
                .dest_size = 6,
                .select    = {
                    .bofst  = 1,
                    .sub_ba = &(struct byte_accessor){
                        .type      = TA_RANGE,
                        .src_size  = 10,
                        .dest_size = 6,
                        .range     = {
                            .sub_ba =
                                &(struct byte_accessor){
                                    .type      = TA_TAKE,
                                    .src_size  = 2,
                                    .dest_size = 2,
                                },
                            .stride = (struct stride){
                                .start  = 0,
                                .stride = 2,
                                .nelems = 3,
                            },
                        },
                    },
                },
            },
        },
    };

    u32 moved = ba_memcpy_from (dest, src, &acc);
    test_assert_int_equal (moved, 6);

    test_assert_int_equal (dest[0], 1); // c[0] = 1
    test_assert_int_equal (dest[1], 0);
    test_assert_int_equal (dest[2], 3); // c[2] = 3
    test_assert_int_equal (dest[3], 0);
    test_assert_int_equal (dest[4], 5); // c[4] = 5
    test_assert_int_equal (dest[5], 0);
  }

  TEST_CASE ("[.a, .a]")
  {
    u8 seq[4] = {0, 1, 2, 3};
    memcpy (src, seq, 4);

    struct byte_accessor dota = {
        .type      = TA_TAKE,
        .src_size  = 4,
        .dest_size = 4,
    };

    u32 moved = ba_memcpy_from (dest, src, &dota);
    test_assert_int_equal (moved, 4);
    moved = ba_memcpy_from (dest + moved, src, &dota);
    test_assert_int_equal (moved, 4);

    test_assert_int_equal (dest[0], 0);
    test_assert_int_equal (dest[1], 1);
    test_assert_int_equal (dest[2], 2);
    test_assert_int_equal (dest[3], 3);
    test_assert_int_equal (dest[4], 0);
    test_assert_int_equal (dest[5], 1);
    test_assert_int_equal (dest[6], 2);
    test_assert_int_equal (dest[7], 3);
  }
}
#endif

static u32
ba_memcpy_to_recursive (u8 *dest, const u8 *src, struct byte_accessor *acc)
{
  switch (acc->type)
  {
    case TA_TAKE:
    {
      memcpy (dest, src, acc->src_size);
      return acc->src_size;
    }
    case TA_SELECT:
    {
      return ba_memcpy_to_recursive (dest + acc->select.bofst, src, acc->select.sub_ba);
    }
    case TA_RANGE:
    {
      u32 elem_size = acc->range.sub_ba->src_size;
      u32 pos       = acc->range.stride.start;
      u32 read      = 0;

      while (pos < acc->range.stride.nelems)
      {
        read += ba_memcpy_to_recursive (dest + pos * elem_size, src + read, acc->range.sub_ba);
        pos += acc->range.stride.stride;
      }

      return read;
    }
  }
  UNREACHABLE ();
}

u32
ba_memcpy_to (u8 *dest, const u8 *src, struct byte_accessor *acc)
{ return ba_memcpy_to_recursive (dest, src, acc); }

#ifndef NTEST
TEST (ba_memcpy_to_basic)
{
  // struct { a int, b struct { b char, c [5]u16 } }
  u8 src[64];
  u8 dest[64];

  TEST_CASE ("[.a]")
  {
    u8 test_data[] = {78, 56, 34, 12};
    memcpy (src, test_data, sizeof (test_data));

    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 4,
        .dest_size = 4,
        .select    = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };

    ba_memcpy_to (dest, src, &acc);

    test_assert_int_equal (dest[0], 78);
    test_assert_int_equal (dest[1], 56);
    test_assert_int_equal (dest[2], 34);
    test_assert_int_equal (dest[3], 12);
  }

  TEST_CASE ("[.b]")
  {
    u8 test_data[] = {0xAB};
    memcpy (src, test_data, sizeof (test_data));

    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 1,
        .dest_size = 1,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 1,
                .dest_size = 1,
            },
        },
    };

    ba_memcpy_to (dest, src, &acc);

    test_assert_int_equal (dest[4], 0xAB);
  }

  TEST_CASE ("[.b, .a]")
  {
    u8 test_data[] = {
        0xAB, // .b
        78,
        56,
        34,
        12 // .a
    };
    memcpy (src, test_data, sizeof (test_data));

    struct byte_accessor dotb = {
        .type      = TA_SELECT,
        .src_size  = 1,
        .dest_size = 1,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 1,
                .dest_size = 1,
            },
        },
    };
    struct byte_accessor dota = {
        .type      = TA_SELECT,
        .src_size  = 4,
        .dest_size = 4,
        .select    = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };

    u32 moved = ba_memcpy_to (dest, src, &dotb); // 0xAB → dest[4]
    test_assert_int_equal (moved, 1);
    moved = ba_memcpy_to (dest, src + moved,
                          &dota); // {78,56,34,12} → dest[0..3]
    test_assert_int_equal (moved, 4);

    test_assert_int_equal (dest[0], 78);
    test_assert_int_equal (dest[1], 56);
    test_assert_int_equal (dest[2], 34);
    test_assert_int_equal (dest[3], 12);
    test_assert_int_equal (dest[4], 0xAB);
  }

  TEST_CASE ("[.a, .b]")
  {
    u8 test_data[] = {
        78,
        56,
        34,
        12,  // .a
        0xAB // .b
    };
    memcpy (src, test_data, sizeof (test_data));

    struct byte_accessor dota = {
        .type      = TA_SELECT,
        .src_size  = 4,
        .dest_size = 4,
        .select    = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };
    struct byte_accessor dotb = {
        .type      = TA_SELECT,
        .src_size  = 1,
        .dest_size = 1,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 1,
                .dest_size = 1,
            },
        },
    };

    u32 moved = ba_memcpy_to (dest, src, &dota);
    test_assert_int_equal (moved, 4);
    moved = ba_memcpy_to (dest, src + moved, &dotb);
    test_assert_int_equal (moved, 1);

    test_assert_int_equal (dest[0], 78);
    test_assert_int_equal (dest[1], 56);
    test_assert_int_equal (dest[2], 34);
    test_assert_int_equal (dest[3], 12);
    test_assert_int_equal (dest[4], 0xAB);
  }

  TEST_CASE ("[.b.c[1:1:4]]")
  {
    u8 test_data[] = {
        2,
        0, // c[1] = 2
        3,
        0, // c[2] = 3
        4,
        0, // c[3] = 4
    };
    memcpy (src, test_data, sizeof (test_data));

    // SELECT(bofst=4, SELECT(bofst=1, RANGE(start=1, stride=1,
    // nelems=4, TAKE(2))))
    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 6,
        .dest_size = 6,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_SELECT,
                .src_size  = 6,
                .dest_size = 6,
                .select    = {
                    .bofst  = 1,
                    .sub_ba = &(struct byte_accessor){
                        .type      = TA_RANGE,
                        .src_size  = 6,
                        .dest_size = 6,
                        .range     = {
                            .sub_ba =
                                &(struct byte_accessor){
                                    .type      = TA_TAKE,
                                    .src_size  = 2,
                                    .dest_size = 2,
                                },
                            .stride = (struct stride){
                                .start  = 1,
                                .stride = 1,
                                .nelems = 4,
                            },
                        },
                    },
                },
            },
        },
    };

    u32 moved = ba_memcpy_to (dest, src, &acc);
    test_assert_int_equal (moved, 6);

    test_assert_int_equal (dest[7], 2); // c[1]
    test_assert_int_equal (dest[8], 0);
    test_assert_int_equal (dest[9], 3); // c[2]
    test_assert_int_equal (dest[10], 0);
    test_assert_int_equal (dest[11], 4); // c[3]
    test_assert_int_equal (dest[12], 0);
  }

  TEST_CASE ("[.b.c[0:2:5]]")
  {
    // Compact src: c[0], c[2], c[4] in linear order
    u8 test_data[] = {
        1,
        0, // c[0] = 1
        3,
        0, // c[2] = 3
        5,
        0, // c[4] = 5
    };
    memcpy (src, test_data, sizeof (test_data));

    // SELECT(bofst=4, SELECT(bofst=1, RANGE(start=0, stride=2,
    // nelems=5, TAKE(2)))) dest offsets: 4+1+(0*2)=5, 4+1+(2*2)=9,
    // 4+1+(4*2)=13
    struct byte_accessor acc = {
        .type      = TA_SELECT,
        .src_size  = 6,
        .dest_size = 6,
        .select    = {
            .bofst  = 4,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_SELECT,
                .src_size  = 6,
                .dest_size = 6,
                .select    = {
                    .bofst  = 1,
                    .sub_ba = &(struct byte_accessor){
                        .type      = TA_RANGE,
                        .src_size  = 6,
                        .dest_size = 6,
                        .range     = {
                            .sub_ba =
                                &(struct byte_accessor){
                                    .type      = TA_TAKE,
                                    .src_size  = 2,
                                    .dest_size = 2,
                                },
                            .stride = (struct stride){
                                .start  = 0,
                                .stride = 2,
                                .nelems = 5,
                            },
                        },
                    },
                },
            },
        },
    };

    u32 moved = ba_memcpy_to (dest, src, &acc);
    test_assert_int_equal (moved, 6);

    test_assert_int_equal (dest[5], 1); // c[0]
    test_assert_int_equal (dest[6], 0);
    test_assert_int_equal (dest[9], 3); // c[2]
    test_assert_int_equal (dest[10], 0);
    test_assert_int_equal (dest[13], 5); // c[4]
    test_assert_int_equal (dest[14], 0);
  }

  TEST_CASE ("[.a, .a]")
  {
    u8 test_data[] = {4, 5, 6, 7};
    memcpy (src, test_data, sizeof (test_data));

    struct byte_accessor dota = {
        .type   = TA_SELECT,
        .select = {
            .bofst  = 0,
            .sub_ba = &(struct byte_accessor){
                .type      = TA_TAKE,
                .src_size  = 4,
                .dest_size = 4,
            },
        },
    };

    u32 moved = ba_memcpy_to (dest, src, &dota);
    test_assert_int_equal (moved, 4);
    moved = ba_memcpy_to (dest, src, &dota);
    test_assert_int_equal (moved, 4);

    test_assert_int_equal (dest[0], 4);
    test_assert_int_equal (dest[1], 5);
    test_assert_int_equal (dest[2], 6);
    test_assert_int_equal (dest[3], 7);
  }
}
#endif
