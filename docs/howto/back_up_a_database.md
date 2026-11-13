Back Up a Database
==================

What is on disk
---------------

A database is one file, plus a log next to it while there is work to recover:

```text
prices.db        the database
prices.db.wal    the write-ahead log
```

After a clean close you are left with just the database file. After a crash the
log is still there, and the next `ns.Database("prices.db")` replays it before
your first query runs - recovery is automatic and needs no flag.

Back up the files
-----------------

To back up a database, copy both files, or copy the database file while
nothing has it open.

Use a tool that knows about sparse files
----------------------------------------

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
