Glossary
========

Terms these docs use.

**ARIES**\
The recovery algorithm Numstore uses. When you open a database, work from
committed transactions is redone and work from transactions that never
committed is undone. See [Durability and crash recovery](concepts/durability.md).

**Automatic transaction**\
The transaction a query gets when you don't give it one. The query happens
completely or not at all, and it's durable by the time the call returns.

**C order**\
How strict arrays are laid out in memory: the last dimension is contiguous.
Also called row-major order. See
[The type system](concepts/types.md#array-order-c-order).

**dtype**\
The NumPy data type of one element. `ns.to_dtype` gives you the dtype of a
Numstore type.

**Plan**\
A compiled query you can run as many times as you like. See
[Reuse a query with a plan](guides/reuse-queries.md).

**Primitive**\
A type with no members, like `u32` or `cf64`. At the bottom of every type is a
primitive.

**Strict array**\
A type with fixed dimensions, for example `[256][256] f32`.

**Struct**\
A type whose members sit back to back, with no padding.

**Union**\
A type whose members are overlaid, all starting at the same byte. Its size is
the size of its largest member.

**Variable**\
A name, a type and one array. A database maps variables to arrays.
