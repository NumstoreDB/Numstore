The Query Language
==================

To get a better glimpse of what Numstore can do, it helps to understand the
Numstore language, which is very self-contained and simple. Everything below is
a real Python session, but the queries are identical in C - only the host
language changes.

* [Start a session](#start-a-session)
* [The seven queries](#the-seven-queries)
* [Variable operations](#variable-operations)
  * [Listing every variable](#listing-every-variable)
* [Array operations](#array-operations)
  * [INSERT](#insert)
    * [Count is a ceiling](#count-is-a-ceiling)
  * [WRITE](#write)
    * [Ranges clamp](#ranges-clamp)
    * [A bare index](#a-bare-index)
  * [READ](#read)
  * [REMOVE](#remove)
* [Next steps](#next-steps)

Start a session
---------------

Open a database ([Getting started](../GETTING_STARTED.md#open-a-database) has
the details). If you want to follow along, type the examples in order - each
section builds on the one before it:

```python
>>> import numpy as np
>>> import pynumstore as ns
>>> db = ns.Database("tour.db")
```

The seven queries
-----------------

Numstore's query language is not SQL. This was a very intentional decision. A
Numstore database has seven fundamental operations:

1. `CREATE`
2. `GET`
3. `DELETE`
4. `INSERT`
5. `WRITE`
6. `READ`
7. `REMOVE`

You could put `CREATE`, `GET` and `DELETE` in their own category of _variable
operations_ and `INSERT`, `WRITE`, `READ` and `REMOVE` in their own category of
_array operations_.

From Python, a query is just a string. Which method you hand it to depends on
what the query needs to move across the boundary - nothing, metadata, data in,
or data out:

| Method               | Queries it accepts         | Returns                        |
| -------------------- | -------------------------- | ------------------------------ |
| `db.execute(q)`      | `CREATE` `DELETE` `REMOVE` | `None`                         |
| `db.get(q)`          | Anything with a variable   | a `Var`, `None` if not exists  |
| `db.get_all()`       | takes no query             | `list[Var]`, one per variable  |
| `db.write(q, data)`  | `INSERT` `WRITE`           | `int` - elements written       |
| `db.read(q)`         | `READ` `REMOVE`            | a fresh `numpy` array          |
| `db.read_into(q, o)` | `READ` `REMOVE`            | `int` - elements read into `o` |

Hand a query to the wrong method and you get an error rather than a surprise:

```python
>>> db.execute("read foo")
Traceback (most recent call last):
  ...
RuntimeError: Only supported exec commands are REMOVE/CREATE/DELETE
```

A query that isn't a query at all fails the same way:

```python
>>> db.execute("not a real query")
Traceback (most recent call last):
  ...
RuntimeError: Expected a valid operation at pos: 0
```

Variable operations
-------------------

`CREATE`, `GET` and `DELETE` exist because a Numstore database is really just a
map of "variables" to "arrays".

A Numstore database may look like this:

```text
"foo"  ->  [1, 2, 3, 4, 5, ...]
"bar"  ->  [1, 3, 8, 9, 10, ...]
"biz"  ->  [3, 6, 7, 8, 9, ...]
...
```

To create new variables you execute the `CREATE` operation. `create` takes a
name and a type, and returns nothing - so in the REPL a successful `create` is
silent:

```python
>>> db.execute("create foo u32")
>>> db.execute("create bar f32")
>>> db.execute("create biz cf128")
```

You can get a variable's information by invoking the `GET` operation.
`db.get` hands you back a `Var`, a frozen snapshot of the variable's metadata:

```python
>>> db.get("get foo")
Var(name='foo', type='u32', length=0, tsize=4)
```

You can also run get for any query that contains a variable in it
(the query never runs - so you only ever read the variable, not
the data)

```python
>>> db.get("read foo[0:10]")
Var(name='foo', type='u32', length=0, tsize=4)
>>> db.get("remove foo[0:10]") # I don't modify foo
Var(name='foo', type='u32', length=0, tsize=4)
```

1. `name` is the variable name
2. `type` is the data type of the array - explained further in
   [The type system](../concepts/types.md)
3. `length` is the number of elements in the array
4. `tsize` is the size in bytes of each element inside the array `foo` (a `u32`
   is 4 bytes)

A `Var` is a dataclass, so the fields are just attributes, and it carries two
numpy-shaped conveniences on top:

```python
>>> var = db.get("get foo")
>>> var.name, var.type
('foo', 'u32')
>>> var.length, var.tsize
(0, 4)
>>> var.dtype
dtype('uint32')
>>> var.shape
(0,)
>>> len(var)
0
```

`var.dtype` is the numpy dtype one element maps to, and `var.shape` is the shape
an array of the whole variable would have - here `(0,)`, because `foo` is still
empty. A `Var` is a snapshot, not a live view: it keeps reporting what was true
when you asked.

`CREATE` is idempotent as long as the type agrees. Re-creating a variable with
the same type leaves it (and its data) alone:

```python
>>> db.execute("create foo u32")
```

Asking for a different type for a name that already exists is an error, not a
silent replacement:

```python
>>> db.execute("create foo f64")
Traceback (most recent call last):
  ...
RuntimeError: Trying to create variable: foo but variable already exists and types are different
```

You can delete a variable by invoking the `DELETE` operation:

```python
>>> db.execute("delete biz")
>>> db.get("get biz")
Traceback (most recent call last):
  ...
RuntimeError: Variable: biz doesn't exist
```

### Listing every variable

`db.get_all` is `get` for the whole database: it takes no query and hands back
one `Var` per variable. It's how you find out what's in a database file you
didn't create. The order isn't specified, so sort if you need a stable list:

```python
>>> sorted(v.name for v in db.get_all())
['bar', 'foo']
>>> {v.name: v.type for v in sorted(db.get_all(), key=lambda v: v.name)}
{'bar': 'f32', 'foo': 'u32'}
```

Array operations
----------------

Each variable has one array. So first, you need to create a variable:

```python
>>> db.execute("create data u32")
>>> db.get("get data")
Var(name='data', type='u32', length=0, tsize=4)
```

The only way to _add_ data to an array (e.g. increase the array size) is to
`INSERT` data into the array. `WRITE` doesn't increase the array size, it only
overwrites data. `INSERT` and `REMOVE` are the only length-modifying
operations.

### INSERT

To `INSERT` data, you specify what _offset_ to insert at and how many elements
you're inserting. The data itself goes to `db.write` as the second argument:

```python
>>> db.write("insert data 0 5", [1, 2, 3, 4, 5])
5
>>> db.read("read data")
array([1, 2, 3, 4, 5], dtype=uint32)
```

The general form of an insert command looks like this:

```text
insert <Variable Name> <Offset> <Count>
```

* _Offset_ (`0` in the example above) is where to insert the data into the
  array. Negative numbers count back from the end of the array, -1 being the
  last index.
* _Count_ (`5` in the example above) is how many elements to insert. It's a
  ceiling, not a promise - see [Count is a ceiling](#count-is-a-ceiling).

The return value is the number of elements actually inserted.

For example, if we insert into a 6 element array at index 2, it's like we're
"pushing" data into index 2:

```text
insert foo 2 3   <- [7, 8, 9]

Before:
  [1, 2, 3, 4, 5, 6]
         ^
     [7, 8, 9]

After:
  [1, 2, 7, 8, 9, 3, 4, 5, 6]
```

```text
insert foo 0 3   <- [7, 8, 9]

Before:
  [1, 2, 3, 4, 5, 6]
   ^
[7, 8, 9]

After:
  [7, 8, 9, 1, 2, 3, 4, 5, 6]
```

```text
insert foo -1 3  <- [7, 8, 9]

Before:
  [1, 2, 3, 4, 5, 6]
                  ^
              [7, 8, 9]

After:
  [1, 2, 3, 4, 5, 7, 8, 9, 6]
```

And here it is for real. `data` is `[1, 2, 3, 4, 5]` from above:

```python
>>> db.write("insert data 2 2", [7, 8])
2
>>> db.read("read data")
array([1, 2, 7, 8, 3, 4, 5], dtype=uint32)
>>> db.write("insert data -1 1", [9])
1
>>> db.read("read data")
array([1, 2, 7, 8, 3, 4, 9, 5], dtype=uint32)
```

A true `append` is an offset equal to the length of the array - or any offset
past it, because offsets clamp:

```python
>>> v = db.get("get data")
>>> db.write(f"insert data {len(v)} 2", [10, 11])
2
>>> db.read("read data")
array([ 1,  2,  7,  8,  3,  4,  9,  5, 10, 11], dtype=uint32)
```

#### Count is a ceiling

The _Count_ in the query and the length of the data you pass don't have to
agree. `INSERT` pairs them up and stops as soon as either one runs out, and the
return value tells you how many elements actually landed:

```python
>>> db.execute("create cap u32")
>>> db.write("insert cap 0 3", [1, 2])
2
>>> db.write("insert cap 0 2", [7, 8, 9])
2
>>> db.read("read cap")
array([7, 8, 1, 2], dtype=uint32)
```

The first call asked for 3 but only had 2 elements to give. The second had 3
elements but only asked for 2, so `9` was never consumed.

### WRITE

While `INSERT` increases the array length, `WRITE` simply overwrites data in
place. To write data, you specify the _range_ you wish to modify:

```python
>>> db.execute("create w u32")
>>> db.write("insert w 0 6", [1, 2, 3, 4, 5, 6])
6
>>> db.write("write w[2:5]", [70, 80, 90])
3
>>> db.read("read w")
array([ 1,  2, 70, 80, 90,  6], dtype=uint32)
```

The general form of a write command looks like this:

```text
write <Variable Name>[ "[" ( index
      | [ start ] ":" [ stop ] [ ":" [ step ] ]
      ) "]" ]
```

Every part of the range is optional:

* _start_ is the first index written to. If it isn't included, it defaults to
  `0`. Negative numbers count back from the end of the array, -1 being the last
  index.
* _stop_ is the index to stop at, **exclusive**. If it isn't included, it
  defaults to the length of the array. Negative numbers count back from the
  end.
* _step_ is the distance between written indexes. If it isn't included, it
  defaults to `1`. A step of `0` is an error.

Assuming `w` is a 6 element array, the following commands all cover the whole
thing:

```python
>>> db.write("write w[0:6:1]", [1, 2, 3, 4, 5, 6])
6
>>> db.write("write w[0:]", [1, 2, 3, 4, 5, 6])
6
>>> db.write("write w[:]", [1, 2, 3, 4, 5, 6])
6
>>> db.write("write w", [1, 2, 3, 4, 5, 6])
6
```

Nothing moves, and the array is the same length afterwards:

```text
write foo[2:5]  <- [7, 8, 9]
Before:
  [1, 2, 3, 4, 5, 6]
         ^  ^  ^
        [7, 8, 9]
After:
  [1, 2, 7, 8, 9, 6]
```

Compare that to `insert foo 2 3`, which pushes the old data out of the way and
leaves you with a 9 element array. If you want to grow an array, you want
`INSERT`.

A _step_ spreads the write out over the array instead of laying it down
contiguously:

```text
write foo[0:6:2]  <- [7, 8, 9]
Before:
  [1, 2, 3, 4, 5, 6]
   ^     ^     ^
   7     8     9
After:
  [7, 2, 8, 4, 9, 6]
```

```python
>>> db.write("write w[0:6:2]", [7, 8, 9])
3
>>> db.read("read w")
array([7, 2, 8, 4, 9, 6], dtype=uint32)
```

A negative _step_ would walk the array backwards. Negative steps aren't
supported yet but are a planned feature:

```python
>>> db.write("write w[0:6:-2]", [7, 8, 9])
Traceback (most recent call last):
  ...
RuntimeError: stride step must be positive
```

A step of `0` is rejected by the same check:

```python
>>> db.write("write w[0:6:0]", [7, 8, 9])
Traceback (most recent call last):
  ...
RuntimeError: stride step must be positive
```

The range and the data don't have to agree on length. `WRITE` pairs them up one
element at a time and stops as soon as either one runs out, and returns how many
elements it wrote. If the data runs out first, the rest of the range is left
exactly as it was:

```python
>>> db.write("write w", [1, 2, 3, 4, 5, 6])
6
>>> db.write("write w[:]", [70, 80])
2
>>> db.read("read w")
array([70, 80,  3,  4,  5,  6], dtype=uint32)
```

And if the range runs out first, the remaining data is simply not consumed:

```python
>>> db.write("write w[0:2]", [11, 12, 13, 14, 15])
2
>>> db.read("read w")
array([11, 12,  3,  4,  5,  6], dtype=uint32)
```

An empty range is legal and writes nothing. In particular, writing to the range
just past the end of an array is **not** an `append`, it's a no-op that returns
`0`:

```python
>>> db.write("write w[6:]", [7, 8, 9])
0
>>> db.read("read w")
array([11, 12,  3,  4,  5,  6], dtype=uint32)
```

#### Ranges clamp

Ranges aren't bounds-checked the way an index into a Python list is. _start_ and
_stop_ are clamped to the array, so a range that hangs off either end just
covers less than you asked for:

```python
>>> db.write("write w[0:100]", [21, 22])
2
>>> db.write("write w[-100:]", [31, 32])
2
>>> db.read("read w")
array([31, 32,  3,  4,  5,  6], dtype=uint32)
```

#### A bare index

A single number in the brackets, with no colon, is a bare index: it covers
exactly one element. The colon is what makes a range open ended, so `w[2]` and
`w[2:]` are different things:

```python
>>> db.read("read w[2]")
array([3], dtype=uint32)
>>> db.read("read w[2:]")
array([3, 4, 5, 6], dtype=uint32)
```

`w[2]` and `w[2:3]` are the same thing. A bare index works wherever a range
does, so it's the way to touch a single element with any of the three
operations:

```python
>>> db.write("write w[2]", [70, 80, 90])
1
>>> db.read("read w")
array([31, 32, 70,  4,  5,  6], dtype=uint32)
```

The write asked to lay down three elements but the range only had room for one,
so only one landed - see the pairing rule above.

A bare index counts from the end like any other, and clamps like any other, so
an index past the end is an empty range rather than an error:

```python
>>> db.read("read w[-1]")
array([6], dtype=uint32)
>>> db.read_into("read w[6]", np.zeros(1, dtype=np.uint32))
0
```

### READ

To `READ` data, you specify the _range_ you wish to read:

```python
>>> db.execute("create r u32")
>>> db.write("insert r 0 6", [1, 2, 3, 4, 5, 6])
6
>>> db.read("read r[2:5]")
array([3, 4, 5], dtype=uint32)
```

The general form of a read command looks like this:

```text
read <Variable Name>[ "[" ( index
     | [ start ] ":" [ stop ] [ ":" [ step ] ]
     ) "]" ]
```

`READ` has the same range semantics as `WRITE`. See [WRITE](#write).

Where `WRITE` puts data into the range, `READ` takes it out. `db.read` allocates
a fresh numpy array of exactly the right dtype and shape and hands it to you.

Assuming `r` is a 6 element array, the following commands do the same thing:

```python
>>> db.read("read r[0:6:1]")
array([1, 2, 3, 4, 5, 6], dtype=uint32)
>>> db.read("read r[0:]")
array([1, 2, 3, 4, 5, 6], dtype=uint32)
>>> db.read("read r[:]")
array([1, 2, 3, 4, 5, 6], dtype=uint32)
>>> db.read("read r")
array([1, 2, 3, 4, 5, 6], dtype=uint32)
```

Reading never modifies the array:

```text
read foo[2:5]
Before:
  [1, 2, 3, 4, 5, 6]
         ^  ^  ^
Data:
  [3, 4, 5]
After:
  [1, 2, 3, 4, 5, 6]
```

A _step_ spreads the read out over the array, the same way it spreads out a
write:

```text
read foo[0:6:2]
Before:
  [1, 2, 3, 4, 5, 6]
   ^     ^     ^
Data:
  [1, 3, 5]
```

```python
>>> db.read("read r[0:6:2]")
array([1, 3, 5], dtype=uint32)
```

Negative numbers count back from the end:

```python
>>> db.read("read r[-3:]")
array([4, 5, 6], dtype=uint32)
```

Negative steps aren't supported yet, so you can't read an array backwards:

```python
>>> db.read("read r[0:6:-2]")
Traceback (most recent call last):
  ...
RuntimeError: stride step must be positive
```

An empty range is legal, but `db.read` has nothing to allocate for it and
raises. If you want "read whatever is there, possibly nothing", use
`db.read_into`, which reports `0`:

```python
>>> db.read("read r[6:]")
Traceback (most recent call last):
  ...
RuntimeError: numstore operation failed
>>> db.read_into("read r[6:]", np.zeros(3, dtype=np.uint32))
0
```

Reading a variable that has no data at all is fine, though - you get an empty
array back:

```python
>>> db.execute("create empty u32")
>>> db.read("read empty")
array([], dtype=uint32)
```

### REMOVE

To `REMOVE` data, you specify the _range_ you wish to remove:

```python
>>> db.execute("create x u32")
>>> db.write("insert x 0 6", [1, 2, 3, 4, 5, 6])
6
>>> db.execute("remove x[2:5]")
>>> db.read("read x")
array([1, 2, 6], dtype=uint32)
```

The general form of a remove command looks like this:

```text
remove <Variable Name>[ "[" ( index
       | [ start ] ":" [ stop ] [ ":" [ step ] ]
       ) "]" ]
```

`REMOVE` has the same range semantics as `READ`. See [READ](#read).

`REMOVE` is the other half of `INSERT`: it's the only operation besides `INSERT`
that changes the length of an array. Everything the range covers is taken out
and whatever is left closes the gap.

`REMOVE` is the one operation that answers to two methods. `db.execute` throws
the removed data away; `db.read` hands it back to you on the way out, which
makes it a destructive read:

```python
>>> db.write("insert x 0 3", [7, 8, 9])
3
>>> db.read("read x")
array([7, 8, 9, 1, 2, 6], dtype=uint32)
>>> db.read("remove x[0:2]")
array([7, 8], dtype=uint32)
>>> db.read("read x")
array([9, 1, 2, 6], dtype=uint32)
```

Either way the variable's length follows the data, so `get` after a remove
reports what's actually left:

```python
>>> db.get("get x")
Var(name='x', type='u32', length=4, tsize=4)
>>> db.execute("remove x[0]")
>>> db.get("get x")
Var(name='x', type='u32', length=3, tsize=4)
>>> db.read("read x")
array([1, 2, 6], dtype=uint32)
```

For example, removing the range `2:5` from a 6 element array takes out three
elements and leaves a 3 element array behind:

```text
remove foo[2:5]
Before:
  [1, 2, 3, 4, 5, 6]
         ^  ^  ^
After:
  [1, 2, 6]
```

A _step_ punches holes in the array instead of taking out one contiguous block.
The array still closes up afterwards:

```text
remove foo[0:6:2]
Before:
  [1, 2, 3, 4, 5, 6]
   ^     ^     ^
After:
  [2, 4, 6]
```

```python
>>> db.execute("remove x[0:3:2]")
>>> db.read("read x")
array([2], dtype=uint32)
```

Because the whole range is optional, the following commands all remove
everything. This empties the variable out, it doesn't get rid of the variable
itself - you're left with an array of length 0:

```text
remove foo
Before:
  [1, 2, 3, 4, 5, 6]
   ^  ^  ^  ^  ^  ^
After:
  []
```

```python
>>> db.execute("remove x")
>>> db.read("read x")
array([], dtype=uint32)
```

An empty range is legal and removes nothing:

```python
>>> db.write("insert x 0 3", [1, 2, 3])
3
>>> db.execute("remove x[6:]")
>>> db.read("read x")
array([1, 2, 3], dtype=uint32)
```

Next steps
----------

* [Transactions](transactions.md) - group these queries into one atomic unit.
* [Type sizes](type_sizes.md) - go beyond arrays of scalars.
* [Move data in and out from Python](../howto/move_data_in_python.md).
