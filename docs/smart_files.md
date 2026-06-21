smartfiles --- C Reference
==========================

A transactional, structured file format for C. Supports
insert / write / read / remove in the middle of an array, multiple
named data streams in a single file, strided access, and WAL-backed
durability.

    #include "smartfiles.h"


Table of contents
-----------------
- [Overview](#overview)
- [Types and constants](#types-and-constants)
- [Core concepts](#core-concepts)
- [API reference](#api-reference)
  - [File lifecycle](#file-lifecycle)
  - [Simple operations](#simple-operations-default-stream)
  - [Parameterized operations](#parameterized-operations-named-streams--strides)
  - [Transactions](#transactions)
- [How-to / cookbook](#how-to--cookbook)
- [Durability guarantees](#durability-guarantees)

Overview
--------

A smart file is a single on-disk file that contains one or more named
data streams ("variables"). Each stream behaves like a dynamic array
of bytes you can insert into, overwrite, read, and remove from at any
offset --- not just append. Operations can be grouped into
transactions with full commit / rollback semantics and survive process
crashes via a write-ahead log.

You interact with a smart file through an opaque handle `smfile_t *`.
There are two API tiers:

  - Simple API --- `smfile_read`, `smfile_write`, `smfile_insert`,
    `smfile_remove`. Operates on the default unnamed stream with
    contiguous byte ranges.

  - Parameterized API --- `smfile_pread`, `smfile_pwrite`,
    `smfile_pinsert`, `smfile_premove`. Adds named streams and
    strided access (skip-N) for power users.

Both tiers share the same transaction machinery and durability
guarantees.


Types and constants
-------------------

`smfile_t`
    Opaque file handle. Always passed by pointer.

`b_size`
    Unsigned byte / offset size type. Used for offsets and lengths.

`sb_size`
    Signed counterpart of `b_size`. Returned by read / remove to
    allow negative error codes and short-read counts.

`SMF_END`
    Sentinel meaning "everything from here to the end of the
    stream." Valid as a length argument to `smfile_read`,
    `smfile_remove`, etc.

`-1` (as offset)
    In parameterized inserts, `-1` as the offset means "append to
    the end of the stream."


Core concepts
-------------

Streams ("variables")

    A single smart file can hold any number of independent named
    streams (for example "temps", "humidity", "pressure"). Each has
    its own length and its own offset space. The simple API uses one
    default unnamed stream; the parameterized API exposes the name
    explicitly.

Insert vs write vs remove

    Insert shifts existing data outward to make room. Length grows.

    Write overwrites in place. Length unchanged (unless writing past
    the end).

    Remove deletes a region and shifts subsequent data inward.
    Length shrinks. Optionally copies the evicted bytes into a
    caller buffer.

    All three are first-class at any offset --- not just the tail.

Strides

    Parameterized operations take an element size and a stride.
    Stride `1` is contiguous; stride `2` skips every other element,
    and so on. Useful for reading or writing every Nth float without
    copying neighbors.

Transactions

    By default, every operation is auto-committed. Wrap a sequence
    in `smfile_begin` / `smfile_commit` to make them atomic.
    `smfile_rollback` undoes everything since the matching
    `smfile_begin`.

Durability

    Committed transactions survive process crashes via a WAL
    replayed on the next `smfile_open`. Uncommitted work is
    discarded. See "Durability guarantees" below.


API reference
=============


File lifecycle
--------------

smfile_open

    smfile_t *smfile_open(const char *path);

Opens (or creates) a smart file at `path`. Returns a handle, or
`NULL` on error. If a WAL exists from a prior crashed process, it is
replayed before this call returns.


smfile_close

    int smfile_close(smfile_t *smf);

Flushes any pending work, closes the file, and frees the handle.
Returns 0 on success. After this call, `smf` is invalid.


smfile_cleanup

    void smfile_cleanup(const char *path);

Deletes the smart file at `path` along with any associated WAL or
sidecar files. Useful at the start of demos and tests to ensure a
clean slate. Safe to call when no such file exists.


Simple operations (default stream)
----------------------------------

These operate on the unnamed default stream with contiguous byte
ranges.


smfile_read

    sb_size smfile_read(smfile_t *smf,
                        void    *dest,
                        b_size   offset,
                        b_size   length);

Reads up to `length` bytes starting at `offset` into `dest`. Returns
the number of bytes actually read (may be less than `length` if the
stream is shorter). Pass `SMF_END` as `length` to read everything
from `offset` onward.


smfile_write

    sb_size smfile_write(smfile_t   *smf,
                         const void *src,
                         b_size      offset,
                         b_size      length);

Overwrites `length` bytes at `offset` with `src`. Does not shift
existing data. Stream length grows only if the write extends past
the current end.


smfile_insert

    sb_size smfile_insert(smfile_t   *smf,
                          const void *src,
                          b_size      offset,
                          b_size      length);

Inserts `length` bytes from `src` at `offset`, shifting existing
bytes outward. Stream length grows by `length`. Inserting in the
middle is a first-class operation, not a copy-then-rewrite.


smfile_remove

    sb_size smfile_remove(smfile_t *smf,
                          void     *dest,
                          b_size    offset,
                          b_size    length);

Removes `length` bytes at `offset` and shifts subsequent bytes
inward. If `dest` is non-NULL, the evicted bytes are copied into it.
Pass `NULL` to discard them. Pass `SMF_END` as `length` to remove
everything from `offset` to the end. Returns the number of bytes
actually removed.


Parameterized operations (named streams + strides)
--------------------------------------------------

These are the power-user equivalents of the simple API. They add two
capabilities: an explicit stream name, and an element-size + stride
layout.

The argument shape is:

    (smf, name, buffer, elem_size, offset, stride, count)

  - `name`      --- stream identifier (string).
  - `elem_size` --- bytes per logical element.
  - `offset`    --- starting BYTE offset.
  - `stride`    --- element stride (1 = contiguous, 2 = every
                    other, etc.).
  - `count`     --- number of elements to operate on.


smfile_pread

    sb_size smfile_pread(smfile_t   *smf,
                         const char *name,
                         void       *dest,
                         b_size      elem_size,
                         b_size      offset,
                         b_size      stride,
                         b_size      count);

Reads `count` elements of `elem_size` bytes each from the named
stream, starting at byte `offset`, advancing by `stride` elements
between reads. Returns the number of elements read.


smfile_pwrite

    sb_size smfile_pwrite(smfile_t   *smf,
                          const char *name,
                          const void *src,
                          b_size      elem_size,
                          b_size      offset,
                          b_size      stride,
                          b_size      count);

Strided overwrite. Same layout semantics as `pread`.


smfile_pinsert

    sb_size smfile_pinsert(smfile_t   *smf,
                           const char *name,
                           const void *src,
                           b_size      offset,
                           b_size      length);

Inserts `length` bytes into the named stream at `offset`. Pass `-1`
as `offset` to append. Creates the stream if it does not yet exist.


smfile_premove

    sb_size smfile_premove(smfile_t   *smf,
                           const char *name,
                           void       *dest,
                           b_size      elem_size,
                           b_size      offset,
                           b_size      stride,
                           b_size      count);

Strided remove. If `dest` is non-NULL, evicted elements are copied
there. May report an error if the named stream does not exist ---
typically safe to ignore when used as a "reset if present" call.


Transactions
------------

Without explicit transaction calls, every operation auto-commits.


smfile_begin

    void smfile_begin(smfile_t *smf);

Starts a transaction. All subsequent mutations are buffered until
`commit` or `rollback`.


smfile_commit

    void smfile_commit(smfile_t *smf);

Atomically applies all buffered mutations. Once this returns, the
transaction is durable --- it will survive a crash and be visible on
the next `smfile_open`.


smfile_rollback

    void smfile_rollback(smfile_t *smf);

Discards all buffered mutations since the matching `begin`. The
stream is left as if none of them happened.


How-to / cookbook
=================


Open, modify, close
-------------------

    smfile_t *smf = smfile_open("mydata");
    if (!smf) return -1;
    smfile_insert(smf, "hello", 0, 5);
    return smfile_close(smf);


Reset a file to empty
---------------------

    smfile_remove(smf, NULL, 0, SMF_END);

Or wipe the file from disk entirely:

    smfile_cleanup("mydata");


Insert into the middle of a string
----------------------------------

    const char *s = "The quick brown fox jumps over the lazy dog";
    smfile_insert(smf, s, 0, strlen(s));

    /* shifts " jumps over..." outward */
    smfile_insert(smf, " really", 34, 7);


Overwrite a region
------------------

    /* replaces 3 bytes at offset 16 */
    smfile_write(smf, "cat", 16, 3);


Remove and capture evicted bytes
--------------------------------

    char evicted[8];
    sb_size n = smfile_remove(smf, evicted, 34, 7);
    evicted[n] = '\0';


Read everything
---------------

    char buf[256];
    sb_size n = smfile_read(smf, buf, 0, SMF_END);
    buf[n] = '\0';


Group operations in a transaction
---------------------------------

    smfile_begin(smf);
    smfile_insert(smf, header, 0,  sizeof(header));
    smfile_insert(smf, body,   8,  sizeof(body));
    smfile_insert(smf, footer, 72, sizeof(footer));
    smfile_commit(smf);

All three appear together --- or none of them do.


Roll back a transaction
-----------------------

    smfile_begin(smf);

    /* would clobber everything */
    smfile_write(smf, zeros, 0, sizeof(zeros));

    /* undo --- original bytes remain */
    smfile_rollback(smf);


Use multiple named streams in one file
--------------------------------------

    smfile_pinsert(smf, "temps",    temps,    0, sizeof(temps));
    smfile_pinsert(smf, "humidity", humidity, 0, sizeof(humidity));
    smfile_pinsert(smf, "pressure", pressure, 0, sizeof(pressure));


Append to a named stream
------------------------

    smfile_pinsert(smf, "temps", more_temps, -1, sizeof(more_temps));


Overwrite one element of a named stream
---------------------------------------

    /* elem_size=1, offset=4, stride=1, count=1 */
    uint8_t correction = 99;
    smfile_pwrite(smf, "temps", &correction, 1, 4, 1, 1);


Read every Nth element (stride)
-------------------------------

    /* 8 floats starting at byte 0, skipping every other one */
    float evens[8];
    sb_size n = smfile_pread(smf, "floats",
                             evens, sizeof(float),
                             0, 2, 8);


Write into alternating slots
----------------------------

    /* starts 1 float in (byte 4), stride 2, 8 elements */
    float neg[8];
    for (int i = 0; i < 8; ++i) neg[i] = -1.0f;
    smfile_pwrite(smf, "floats",
                  neg, sizeof(float),
                  4, 2, 8);


Best-effort reset of a named stream
-----------------------------------

    /* safe to ignore the error if the stream did not yet exist */
    smfile_premove(smf, "floats", NULL, 1, 0, 1, SMF_END);


Durability guarantees
=====================

smartfiles uses a write-ahead log. The contract is:

    Scenario                            On next smfile_open
    --------                            -------------------
    Committed, clean exit               visible
    Committed, then process crashed     visible (replayed)
    Not committed, process crashed      discarded
    Explicitly rolled back              discarded

This means:

  1. A successful `smfile_commit` is durable. Once it returns, the
     change will be visible after any subsequent crash + reopen.

  2. A crash before `smfile_commit` is a no-op. Any partial work
     from inside the open transaction vanishes.

  3. `smfile_rollback` is symmetric. The stream is indistinguishable
     from one where the `begin` ... `rollback` block never ran.

  4. `smfile_open` performs WAL recovery. If a prior process
     committed but crashed before the WAL was applied to the main
     file, the open call replays it before returning the handle.

These guarantees apply to both the simple and parameterized APIs
uniformly.
