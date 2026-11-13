Glossary
========

These documents use each word below with one meaning only.

**ARIES**\
The recovery method that Numstore uses. On open, it redoes the work of
committed transactions and undoes the work of transactions that did not
commit. See [Durability and crash recovery](concepts/durability.md).

**Automatic transaction**\
The transaction that a query gets when you do not give it one. The query
happens fully or not at all. It is durable when the call returns.

**C order**\
The memory order of strict arrays. The last dimension is contiguous. Also
called row-major order. See [The type system](concepts/types.md#array-order-c-order).

**Commit**\
The end of a transaction that keeps its work. When `commit` returns, the
work is on disk.

**Count**\
The number in an `insert` query that sets the most elements to insert. The
count is a ceiling. See
[Count is a ceiling](tutorials/language_tour.md#count-is-a-ceiling).

**Database**\
One file that holds variables. While there is work to recover, a log file is
next to it.

**dtype**\
The numpy data type of one element. `ns.to_dtype` gives the dtype of a
Numstore type.

**Durable**\
On disk. A crash cannot undo durable work.

**Element**\
One item of a variable's array. Its size is the size of the variable's type.

**Handle**\
A pointer that the C API gives you: a database (`nsdb_t`), a transaction
(`nsdb_txn_t`), a variable (`nsdb_var_t`) or a plan (`nsdb_plan_t`).

**Length**\
The number of elements in a variable.

**Offset**\
The index where an `insert` starts. A negative offset counts from the end.

**Plan**\
A compiled query that you can run many times. See
[Reuse a query with a plan](howto/reuse_queries.md).

**Primitive**\
A type with no members, for example `u32` or `cf64`. Every type is made of
primitives.

**Query**\
A short text command, for example `insert prices 0 3`. There are seven:
`create`, `get`, `delete`, `insert`, `write`, `read` and `remove`.

**Range**\
The part of a variable that `write`, `read` and `remove` touch. It has the
form `[start:stop:step]`. Each part is optional.

**Rollback**\
The end of a transaction that discards its work.

**Smart file**\
A file with one untyped byte array and no variables. It has the same ACID
guarantees as a database. See [Use smart files](howto/use_smart_files.md).

**Sparse file**\
A file whose apparent size is larger than the disk space that it uses. A
Numstore database is a sparse file.

**Step**\
The distance between indexes in a range. It must be positive. Also called
stride.

**Strict array**\
A type with fixed dimensions, for example `[256][256] f32`.

**Struct**\
A type whose members are one after the other, with no padding.

**Transaction**\
A group of queries that happen together or not at all. One transaction is
open at a time.

**tsize**\
The size of one element, in bytes.

**Union**\
A type whose members all start at the same byte. Its size is the size of its
largest member.

**Variable**\
A name, a type and one array. A database maps variables to arrays.

**WAL (write-ahead log)**\
The log file next to the database. Numstore writes each change to the log
before it writes the change to the database.
