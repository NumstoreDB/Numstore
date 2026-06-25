Numstore Language Spec
======================
At the core of Numstore is the Language Spec
A Numstore database is a hash map of arrays:
```
"foo"  ->  [1, 2, 3, 4, 5, ...]
"bar"  ->  [1, 3, 8, 9, 10, ...]
"biz"  ->  [3, 6, 7, 8, 9, ...]
...
```
Furthermore, each variable has an associated type with it:
```
"foo: (u32)"                   ->  [1, 2, 3, 4, 5, ...]
"bar: (f32)"                   ->  [1.0, 2.0, ...]
"biz: (struct { a f32, b u32 } ->  [ { a: 1.0, b : 10 }, { a: 5.1, b : 1 } ... ]
...
```
(See [types])

Reading
=======
```
read <VNAME> [RANGE]
```
Reads elements from `<VNAME>` into the data buffer passed to `nsdb_execute`.
In the interactive shell the result is rendered to the console (truncated
with dots when long).

Examples
* Read all of foo to the console (with dots):
```
read foo;
```

* Read a subset of foo with start index 10 and end index 100:
```
read foo [10:100];
```

Writing
=======
```
> write <VNAME> [RANGE];
```
Overwrites the elements of `<VNAME>` at `[RANGE]` with data taken from the
buffer passed to `nsdb_execute`. The number of elements written is determined
by the length of `[RANGE]`; it is an error for the buffer to be shorter than
that. `write` is atomic — either every element lands or none of them do.

Examples
* Overwrite the first 100 elements of foo:
```
> write foo [:100];
```
* Overwrite a slice in the middle:
```
> write foo [1000:2000];
```

Inserting
=========
```
> insert <VNAME> <OFFSET>;
```
Inserts new elements into `<VNAME>` starting at element `OFFSET`, shifting
existing elements forward. The data is taken from the buffer passed to
`nsdb_execute`, and the number of elements inserted is determined by the
buffer length. `insert` is atomic.

`OFFSET` may be negative (counted from the end) or `NS_END` to append.

Examples
* Prepend:
```
> insert foo 0;
```
* Insert at element 100:
```
> insert foo 100;
```
* Append:
```
> insert foo NS_END;
```

Removing
========
```
> remove <VNAME> [RANGE] to <DEST>;
```
Where DEST can be:
* A file
```
> remove <VNAME> [RANGE] to out.bin;
```
* Another non sink query
```
> remove <VNAME> [RANGE] to (insert <VNAME> <OFFSET>);
> remove <VNAME> [RANGE] to (write <VNAME> [RANGE]);
```
* Ignore
```
> remove <VNAME> [RANGE];
```
* To the console
```
> remove <VNAME> [RANGE] to console;
```

Variables
=========
```
> create <VNAME> <TYPE>;
> delete <VNAME>;
> get <VNAME GLOB>;
```

`create` registers a new variable with the given type. The array starts
empty; use `insert` to populate it. It is an error to create a variable
that already exists.

`delete` removes a variable and all of its data. This is distinct from
`remove`, which only deletes elements from an existing variable's array.

`get` lists every variable whose name matches `<VNAME GLOB>`, along with
its type and length. The glob supports `*` (any sequence of characters)
and `?` (any single character).

Examples
* Create a variable of unsigned 32-bit integers:
```
> create foo u32;
```
* Create a struct-typed variable:
```
> create samples struct { t f64, v f32 };
```
* Delete a variable:
```
> delete foo;
```
* List every variable:
```
> get *;
```
* List every variable whose name starts with `temp_`:
```
> get temp_*;
```

Meta Commands
=============
```
> exit;
> help;
> help [COMMAND];
```

`exit` closes the current shell session. In-flight transactions are rolled
back on exit.

`help` with no argument prints a one-line summary of every command. With
a command name, it prints the full syntax and a short description of that
command (e.g. `help read;`).

Types
=====
Primitives
----------
Unsigned integers, by bit width:
```
u8   u16   u32   u64
```
Signed integers, by bit width:
```
i8   i16   i32   i64
```
Floating point, by bit width:
```
f32   f64   f128   f256
```
Complex numbers with unsigned integer components, total bit width:
```
cu16   cu32   cu64   cu128
```
Complex numbers with signed integer components, total bit width:
```
ci16   ci32   ci64   ci128
```

Structs
-------
```
STRUCT ::= struct { <LABEL> <TYPE>, <LABEL> <TYPE>, .... }
```
A struct is a fixed, ordered collection of named fields. Fields are laid
out contiguously in declaration order; the size of the struct is the sum
of its field sizes. Field types may themselves be composite.

Examples
* A point in 2D:
```
struct { x f32, y f32 }
```
* A timestamped reading:
```
struct { t f64, v f32, flags u8 }
```
* Nesting:
```
struct { p struct { x f32, y f32 }, label u32 }
```

Unions
------
```
UNION  ::= union  { <LABEL> <TYPE>, <LABEL> <TYPE>, .... }
```
A union holds exactly one of its variants at a time. The on-disk size of
a union is the size of its largest variant plus a tag identifying which
variant is currently inhabited.

Examples
* Either a count or an error code:
```
union { ok u32, err i32 }
```

Strict Array
------------
```
SARRAY ::= [DIM][DIM]...[DIM] <TYPE>
```
A strict array is a fixed-shape, multi-dimensional array of a single
element type. Each `[DIM]` is a compile-time constant length. The total
element count is the product of every dimension, and the on-disk size is
that count times the size of the element type. Strict arrays are stored
in row-major order (rightmost dimension varies fastest).

Examples
* A 3x3 matrix of f32:
```
[3][3] f32
```
* A length-16 vector of u8 (e.g. a fixed-width hash):
```
[16] u8
```
* An array of structs:
```
[4] struct { x f32, y f32 }
```

Ranges
======
Most commands accept a `[RANGE]` to address a slice of a variable's array.
```
[START:END]
```
Both bounds are optional and default to the start / end of the array. Bounds
are element-indexed, not byte-indexed.

Examples
* All elements (equivalent to omitting the range entirely):
```
> read foo [:];
```
* From element 10 to element 99 (end is exclusive):
```
> read foo [10:100];
```
* From element 10 to the end:
```
> read foo [10:];
```
* The first 100 elements:
```
> read foo [:100];
```
A negative `START` or `END` counts from the end of the array. The sentinel
`NS_END` (see numstore.h) is accepted in API form when the end of the array
is what you want.

