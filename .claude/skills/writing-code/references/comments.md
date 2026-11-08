Comments
========

The code is the main documentation. Names, structure and context carry the
meaning. A comment says why, or names a step of an algorithm. It doesn't
repeat the line below it.

Rules
-----

1. Don't duplicate the code. `i = i + 1; // add one to i` makes the file worse.
   It's clutter, and it goes stale.
2. If a variable needs a comment to explain what it holds, rename it. Rewrite
   bad code instead of commenting it.
3. If you can't write a clear comment, the code is probably wrong. Simplify
   it until you can.
4. No in-jokes or unexplained abbreviations. If an abbreviation can't be
   avoided, define it once.
5. Explain code that looks wrong. If something looks redundant but is
   required (a second flush, an extra latch release after a crash), say why,
   or someone will "simplify" it.
6. Cite sources. Copied or adapted code gets a link. Algorithms from papers
   get the reference, e.g. `// ARIES Fig. 10: restart analysis`.
7. Comment bug fixes. When a fix looks odd, record what broke and why, and
   point at the regression test that covers it.
8. Mark unfinished work as `// TODO: <what and why>`, specific enough that
   someone else could pick it up. `// TODO: make it work` is useless.
9. When you change code, update or delete the comments around it. A stale
   comment is worse than none.

Where comments go
-----------------

- Headers are for callers: what the function does, what the parameters mean,
  what it returns on success and on failure. Use `///` and refer to
  parameters as `[name]`.
- `.c` files are for people reading the implementation: the steps of the
  algorithm, invariants, and the reasons behind non-obvious choices.
- Each step of a long function (a state machine, a multi-step protocol) gets
  a short marker comment, so the function can stay in one piece. See
  [simplicity-and-functions.md](simplicity-and-functions.md).
- Tests get a comment saying what property they check, and long ones get
  Setup / Run / Teardown markers. 

Good comments
=============

Naming the steps of a well known protocol
-----------------------------------------

`libs/numstore/src/nscore/pager/ns_pgr_commit.c`, `pgr_commit`:

```c
  // COMMIT
  lsn l = wal_append_commit_log (p->ww, tx->tid, tx->data.last_lsn);

  // FLUSH
  wal_flush_all (p->ww);

  // END
  l = wal_append_end_log (p->ww, tx->tid, l);

  // Remove the transaction from the txn table
  txnt_remove_txn_expect (p->tnxt, tx);

  // Unlock all locks from the txn (2PL shrinking phase)
  lockt_unlock_tx (p->lt, tx);
```

`COMMIT / FLUSH / END` are the ARIES steps, so a reader can check the order
against the paper. "2PL shrinking phase" explains why the unlock comes last.
The function stays in one piece because the markers do the job a helper
function would.

Explaining code that looks wrong
--------------------------------

`libs/numstore/src/core/os/posix/ns_posix.c`, `sys_prealloc`:

```c
  // macOS has no posix_fallocate. F_PREALLOCATE reserves blocks but leaves
  // the size alone, so the size has to be set separately - and because that
  // step is an ftruncate, which shrinks as readily as it grows, a file that
  // is already big enough has to be left alone entirely.
```

The early return for "already big enough" looks like an optimization someone
could delete. The comment says it's what stops the file from shrinking.

Showing the arithmetic is safe
------------------------------

`libs/numstore/src/core/os/windows/ns_windows.c`:

```c
  // Split into whole seconds + remainder so elapsed * 1e9 can't overflow.
  // rem < freq, so rem * 1e9 is safe for any frequency below ~9.2 GHz.
  const i64 sec = elapsed / freq;
  const i64 rem = elapsed % freq;
```

It gives the reason for the odd split, and the bound that makes it correct.

Ownership and lifetime
----------------------

`libs/pynumstore/ns_pynumstore.c`:

```c
  // The database only keeps a pointer to its error, so that error has to
  // outlive this call - _pyns_db_close frees it again
  error *persistent = mem.table->malloc (mem.self, 1, sizeof *persistent, e);
```

Without it, a heap allocation for one `error` looks like a mistake. It also
says where the matching free is.

Why a design choice was made
----------------------------

`libs/numstore/src/core/os/faulty/ns_faulty_os.c`:

```c
// The i_mem counterpart of the faulty OS
// It is a separate decorator because i_mem is a separate interface -
// a simulation wants to inject OOM independently of IO faults.
```

It answers the obvious question ("why not one faulty decorator?") before
anyone asks it.

Recording a bug fix
-------------------

`libs/numstore/src/core/ns_gr_lock.c`, above `gr_lock_is_x_blocks`:

```c
// This used to sleep 50ms and then sample "is thread 2 still blocked?", which
// is not an invariant on a loaded machine - it failed on macOS. It now waits
// for thread 2 to actually be parked on the lock's waiter list and holds
// thread 1 until the sample is taken, so there is no timing assumption left.
```

It records what the old version did, how it failed, and why the new one can't
fail the same way. That stops someone from putting the sleep back.

`libs/numstore/src/nscore/testing/regressions/0002_create_crash_close_delete.c`:

```c
  // Delete the variable
  //
  // This Failed - it shouldnt
  //        CAUSE:
  //          The first log of fsm is a fsm update log. But the fsm page starts
  //          uninitialized, therefore it needs one upfront physical log first
  //          before it can be used - log a physical update log then continue on
  //          with fsm specific logs
  res = ns_execute (db, NULL, "delete MkWMJ9a");
```

The `CAUSE:` sits on the exact step that used to fail.

Why a test exists
-----------------

`libs/numstore/src/core/ns_crc.c`:

```c
// Splitting a buffer anywhere must not change the result, which is what lets
// checksum_execute be called incrementally
TEST (crc_sw_is_resumable_at_every_split)
```

It names the property under test and the caller that depends on it.

Comments to delete
------------------

```c
  // Create chunk
  struct chunk *new_chunk = chunk_create (size, ca->settings.mem, e);

  // Add to front of list
  new_chunk->next = ca->head;

  // Clamp to minimum
  if (new_chunk_size < ca->settings.min_chunk_size) {

  (void)e; // Unused
```

The last clamp in the same function was kept and rewritten to say why:

```c
  // A request larger than max_chunk_size still gets a chunk of its own
  if (new_chunk_size < alloc_size) {
    new_chunk_size = alloc_size;
  }
```
