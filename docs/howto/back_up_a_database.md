Back Up a Database
==================

What is on disk
---------------

A database is one file. While there is work to recover, a log file is next to
it:

```text
prices.db        the database
prices.db.wal    the write-ahead log
```

After a correct close, only the database file stays. After a crash, the log
stays. The next `ns.Database("prices.db")` replays the log before your first
query runs. Recovery is automatic. It needs no flag.

Back up the files
-----------------

To back up a database, do one of these:

* Copy both files.
* Copy the database file while no process has it open.

Use a tool that knows about sparse files
----------------------------------------

**Note:** the database file is **sparse**. A database that holds three numbers
shows a size of about 128 MiB. But it uses about 32 KiB of disk:

```sh
ls -l prices.db    # 133791768 bytes
du -h prices.db    #  32K
```

That size is address space that Numstore allocated before use. It is not data.
Some tools know about holes: `du`, `rsync -S`, and `cp` on Linux and macOS.
These tools see the real size. Other tools make the copy as large as the full
apparent size.
