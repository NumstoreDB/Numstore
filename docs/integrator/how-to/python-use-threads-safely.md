# Python: Use Threads Safely

The practical consequence today is that you must finish a transaction before
starting the next one on the same handle. Open a second one without closing the
first and `begin()` blocks forever waiting for a transaction only you can
finish:

```python
# Don't do this: the second begin() waits for the first, which never commits.
txn1 = db.begin()
txn2 = db.begin()   # blocks
```

The same applies to two threads sharing one `Database`: the waiting `begin()`
does not currently release the GIL, so the thread holding the open transaction
never gets to commit. One transaction at a time, start to finish, is the
supported pattern:

```python
# Fine: each transaction is finished before the next one starts.
for batch in batches:
    with db.begin() as txn:
        txn.write("insert stream 0 %d" % len(batch), batch)
```

Opening the same file twice is also not a way around this. Two handles on one
file both open, but the second does not see the first's commits:

```python
>>> a = ns.Database("shared.db")
>>> a.execute("create v u32")
>>> a.write("insert v 0 2", [1, 2])
2
>>> b = ns.Database("shared.db")      # opens, but is not coherent with a
>>> a.write("insert v 2 1", [3])
1
>>> b.read("read v")                  # a's third element is missing
array([1, 2], dtype=uint32)
>>> a.close(); b.close()
```

Open one `Database` per file, per process, and use it serially. Concurrent
readers and writers across threads and processes are in development; until then
treat a database file as owned by one handle at a time.
