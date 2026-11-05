# Durability and Crash Recovery

[`samples/sample4_crash.py`](https://github.com/NumstoreDB/Numstore/blob/develop/samples/sample4_crash.py) demonstrates this with
real process crashes. It forks a child per phase and kills it with
`os._exit(1)`, which gives the library no chance to flush or clean up:

```sh
python samples/sample4_crash.py
```

```text
full contents: 'AAABBAAAAAAA'
phase 2 - commit + crash      (expect BB): 'BB'
phase 3 - no commit + crash   (expect AA): 'AA'
```

The three phases are:

1. Insert ten `A` bytes and close cleanly. This is the baseline.
2. Insert `BB` at offset 3, **commit**, then crash. The commit happened, so the
   log is replayed on the next open and `BB` is there.
3. Insert `CC` at offset 7 and crash **without committing**. That work is undone
   on the next open, so those bytes still read `AA` - and the array is 12
   elements, not 14.

## What's on disk

A database is one file, plus a log next to it while there is work to recover:

```text
prices.db        the database
prices.db.wal    the write-ahead log
```

After a clean close you are left with just the database file. After a crash the
log is still there, and the next `ns.Database("prices.db")` replays it before
your first query runs - recovery is automatic and needs no flag. To back up a
database, copy both files, or copy the database file while nothing has it open.

One surprise worth knowing about: the database file is **sparse**. A database
holding three numbers reports a size of about 128 MiB but occupies around 32 KiB
of actual disk:

```sh
ls -l prices.db    # 133791768 bytes
du -h prices.db    #  32K
```

That is preallocated address space, not data. Tools that understand holes
(`du`, `rsync -S`, `cp` on Linux and macOS) see the real size; tools that don't
will inflate the copy to the full apparent size.

## ACID properties

`commit()` is the durability point. When it returns, the data is on disk and a
crash cannot take it back. `db.close()` is still worth calling on a clean exit,
but it isn't what makes committed data durable.

Numstore writes ahead to a log and recovers with ARIES on the next open: work
from a committed transaction is redone, work from a transaction that never
committed is undone. There is no third outcome, which is the whole point - no
state exists where half of a transaction is visible.
