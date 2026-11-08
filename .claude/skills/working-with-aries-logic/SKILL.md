---
name: working-with-aries-logic
description: The ARIES recovery algorithm (WAL, LSNs, CLRs, repeating history, analysis/redo/undo, fuzzy checkpoints, nested top actions) and how it maps onto Numstore's pager, WAL, transaction table and dirty page table. Use whenever you read, change, debug or test logging, commit, rollback, checkpoint, crash or restart recovery code (libs/numstore/src/nscore/pager, wal, txn_table, dpg_table), or reason about whether a log or page state is correct after a crash.
---

Working with ARIES logic
========================

Numstore's recovery follows ARIES (Mohan et al., 1992). The full paper is at
<https://web.stanford.edu/class/cs345d-01/rl/aries.pdf>. This file is the
working summary

When the summary isn't enough, open the paper at the section named.

Summary
-------

Log every page update with an LSN, stamp that LSN on the page, and after a
crash **repeat history** (redo everything, losers included), then undo the
losers, writing a **CLR** for every undo whose **UndoNxtLSN** skips work
already undone. (Section 13.)

Core concepts
-------------

| Concept       | Meaning                                                                    | Ref     |
| ------------- | -------------------------------------------------------------------------- | ------- |
| LSN           | Monotonic address of a log record                                          | 1.1     |
| PrevLSN       | Back-pointer to the same txn's previous record                             | 1.1, 4.1|
| page_LSN      | LSN of the last record whose update is on this page                        | 4.2     |
| CLR           | Redo-only record describing an undo; never undone itself                   | 3, 4.1  |
| UndoNxtLSN    | In a CLR: the PrevLSN of the record it compensates (next thing to undo)    | 3, 4.1  |
| Txn table     | TransID, State (P/U), LastLSN, UndoNxtLSN                                  | 4.3     |
| Dirty pages   | PageID to RecLSN (first LSN that may have dirtied it since last flush)     | 4.4     |
| RedoLSN       | min RecLSN in the dirty page table after analysis; where redo starts       | 6.1     |
| Loser         | Txn neither committed nor prepared at the crash                            | 3       |
| Master record | Well-known location holding the last complete begin_chkpt LSN              | 5.4     |

Invariants
----------

1. **WAL rule.** A page may only reach disk after the log up to its page_LSN is
   on stable storage. (1.1, 1.4)
2. **Commit rule.** A txn is committed only once the log is forced through its
   commit record. (1.1)
3. **Log order = page order.** Hold the page latch across the call to the
   logger, so the log order of updates to a page matches the order they were
   applied. (5.1)
4. **page_LSN is the truth.** It is only meaningful because history is
   repeated. Redo a record iff `page_LSN < rec.LSN`. Undo is *not*
   conditional on page_LSN. (3, 6.2, 6.3, 10.1)
5. **CLRs are never undone.** When rollback meets a CLR it just jumps to its
   UndoNxtLSN. Each undoable record gets at most one CLR, no matter how many
   crashes or nested rollbacks happen. (3, 5.2, Fig. 5)
6. **Split undo/redo records:** write the undo-only record first, and put the
   redo-only record's LSN on the page. (5.1)
7. **Rollback takes no locks**, only latches, so a rolling-back txn can never
   deadlock. (5.2)
8. **Redo writes no log records.** Undo writes CLRs, and an end record when a
   txn is fully undone. (6.2, 6.3)
9. **Fuzzy checkpoints need a conservative redo point.** Redo must start at
   `min(min RecLSN in end_chkpt, LSN(begin_chkpt))`, because the dirty page
   list can be stale by the time end_chkpt is written. (5.4, 6.1) Numstore's
   checkpoints are sharp instead; see the differences below.
10. **Space-map / free-space updates are redo-only, with logical undo.** Undo
    of a data update recomputes the free-space state and logs a CLR for it,
    rather than blindly inverting the original. (10.3)

Restart recovery
----------------

Three passes, in this order (Fig. 9):

1. **Analysis** (Fig. 10), forward from the last complete begin_chkpt:
   rebuild the txn table and dirty page table. New txn: add as 'U'.
   Update/CLR: set LastLSN; UndoNxtLSN = rec.LSN for undoable updates, or the
   CLR's UndoNxtLSN; add the page to dirty pages with RecLSN = rec.LSN if
   absent. end_chkpt: merge its tables. prepare: state 'P'. end: drop the txn.
   Finally write end records for txns that had fully rolled back
   (UndoNxtLSN = 0) and compute RedoLSN.
2. **Redo** (Fig. 11), forward from RedoLSN: for each redoable update or CLR,
   skip if the page isn't in dirty pages or `rec.LSN < RecLSN`; otherwise fix
   the page and redo iff `page_LSN < rec.LSN`, then set page_LSN. Redo
   *everything*, including losers.
3. **Undo** (Fig. 12), one backward sweep: repeatedly take the loser with the
   max UndoNxtLSN. Update: undo it, write a CLR with UndoNxtLSN = rec.PrevLSN,
   set page_LSN to the CLR's LSN; if PrevLSN = 0, write end and drop the txn.
   CLR: jump to its UndoNxtLSN. Never consult page_LSN to decide whether to
   undo.

Then take a checkpoint. Checkpoints may also be taken during each pass (7).

Normal rollback (Fig. 8) is the same loop as restart undo, for one txn, down
to a SaveLSN.

Why ARIES is shaped this way
----------------------------

- **Why repeat history?** Selective redo (redo only winners) lets a winner's
  later update push page_LSN past a loser's earlier one, so undo can no longer
  tell whether the loser's update is on the page. Undo-before-redo fails the
  mirror-image way. (10.1, Figs. 15, 16)
- **Why CLRs at all?** They make the state "march forward" during rollback,
  allow logical undo (undo on a different page than the original), let media
  recovery treat rollbacks like any other update, and allow early lock release.
  (10.2, 13)
- **Why UndoNxtLSN?** Without it, systems undo CLRs or undo the same record
  repeatedly; logging grows linearly or exponentially with repeated crashes.
  (Fig. 4, 11)
- **Why one LSN per page, not per record?** Repeating history makes one
  enough, and per-object LSNs waste space and don't work for variable-length
  data. (10.4)

Nested top actions
------------------

To make some work permanent even if the enclosing txn rolls back (e.g. file
extension or a structural change others may build on): remember the txn's
LastLSN, log the actions as normal undo-redo records, then write a **dummy
CLR** whose UndoNxtLSN points to the remembered LSN. A crash before the dummy
CLR undoes the partial action; after it, rollback skips it. A single-record
action can instead just be logged redo-only. Relies on repeating history.
(9, Fig. 14)

Where it lives in Numstore
--------------------------

| ARIES                         | Numstore                                                        |
| ----------------------------- | --------------------------------------------------------------- |
| Log, LSNs, forcing            | `nscore/wal/` (`wal_append_*`, `wal_flush_all`)                 |
| Record types                  | `nscore/wal/ns_wal_record.h`: `WL_BEGIN`, `WL_COMMIT`, `WL_END`, `WL_UPDATE`, `WL_CLR` (physical, fsm, file-extend and dummy CLR variants) |
| PrevLSN / UndoNxtLSN          | `.prev` / `.undo_next` on the write structs                     |
| page_LSN                      | `page_get_page_lsn` / `page_set_page_lsn` (`nscore/page/ns_page.h`) |
| Txn table                     | `nscore/txn_table/` (`struct txn_data`: `last_lsn`, `undo_next_lsn`, `state`) |
| Dirty page table              | `nscore/dpg_table/` (`dpgt_add`, `dpgt_min_rec_lsn`, ...)       |
| Begin / commit / end          | `pgr_begin_txn`, `pgr_commit` (`nscore/pager/`)                 |
| Rollback to savepoint         | `pgr_rollback (p, tx, save_lsn, e)`                             |
| Checkpoint (not fuzzy, see below) | `ns_pgr_checkpoint.c` (`pgr_launch_checkpoint_thread`), `pgr_write_next_lsn` |
| Restart                       | `pgr_recover`, `pgr_detect_wal_and_recover`                     |
| Analysis / redo / undo        | `pgr_restart_analysis`, `pgr_restart_redo`, `pgr_restart_undo`  |
| Free-space map (10.3)         | fsm update / fsm CLR records                                    |

Differences from the paper to keep in mind:

- **Checkpoints are sharp, not fuzzy.** The periodic checkpoint takes an X
  lock on the whole database, flushes every dirty page, flushes the WAL,
  records the next start LSN (`pgr_write_next_lsn`), then deletes and
  reopens the WAL starting at that LSN. After a checkpoint there is nothing
  to redo or undo before it, so the paper's begin_chkpt/end_chkpt and
  "RedoLSN = min(...)" machinery (5.4, invariant 9) doesn't apply to it.
- Numstore writes an explicit `WL_BEGIN` record (the paper uses PrevLSN = 0
  instead).
- There is no two-phase commit, so no prepared/in-doubt state. Txn states are
  `TX_RUNNING`, `TX_CANDIDATE`, `TX_COMMITTED`, `TX_DONE`.

Check the code before assuming anything else matches the paper exactly.

When changing recovery code
---------------------------

1. Name the invariant(s) above that the change touches, and check each one.
2. Walk the change through a crash at every step: before the log force, after
   it, mid-rollback, mid-restart (crash during recovery must be safe too).
3. Ask: after redo, does every page reflect exactly the logged history? After
   undo, does each loser record have exactly one CLR?
4. Test with crashes, not just clean runs: `ns_aries_tests.c`, a regression in
   `testing/regressions/`, fail_after sweeps (see
   [writing-c-unit-tests](../writing-c-unit-tests/SKILL.md) and
   [error-tests.md](../writing-c-unit-tests/references/error-tests.md)), and
   `make test-simtest`.
