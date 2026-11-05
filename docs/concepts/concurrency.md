# Concurrency Model

Numstore serializes transactions strictly: **one transaction is open at a time
per database handle**, and `begin()` waits for the open one to finish before it
returns. That is what makes transactions strictly serializable - the schedule is
the order in which they commit, with no interleaving to reason about.

Numstore serializes transactions strictly: one transaction is open at a time per
database handle, and `begin()` waits for the open one to finish before it
returns. That is what makes transactions strictly serializable - the schedule is
the order they commit in, and there is no interleaving to reason about.

So finish each transaction before you open the next one. Calling `db.begin()`
while another transaction is still open means waiting forever on a transaction
only you can finish.

The same is true of two threads sharing one `Database`: the waiting `begin()`
does not currently release the GIL, so the thread holding the transaction never
gets to commit it. And opening the same file twice is not a way around it -
both handles open, but the second doesn't see the first's commits.

One handle per file, one transaction in flight, start to finish. Concurrent
readers and writers across threads and processes are in development; see
[Concurrency and threading](#concurrency-and-threading) for what
holds today.
