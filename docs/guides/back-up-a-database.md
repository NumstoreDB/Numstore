Back up a database
==================

What's on disk
--------------

A database is one file, plus a log next to it while there's work to recover:

```text
prices.db        the database
prices.db.wal    the write-ahead log
```

After a clean close, only the database file is left. After a crash the log is
still there, and the next `ns.Database("prices.db")` replays it before your
first query runs. You don't need a flag for that.

Back up the files
-----------------

To back up a database, copy both files, or copy the database file while
nothing has it open.

Use a tool that knows about sparse files
----------------------------------------

The database file is sparse. A database that holds three numbers reports a size
of about 128 MiB but only takes up around 32 KiB on disk:

```sh
ls -l prices.db    # 133791768 bytes
du -h prices.db    #  32K
```

Most of that is preallocated address space with no data in it. Tools that
understand holes (`du`, `rsync -S`, `cp` on Linux and macOS) see the real size.
Tools that don't inflate the copy to the full apparent size.
