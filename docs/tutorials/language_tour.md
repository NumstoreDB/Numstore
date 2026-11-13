The Query Language
==================

This tutorial teaches the Numstore query language. It uses Python. The queries
are the same in C.

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

Open a database. [Getting started](../GETTING_STARTED.md#open-a-database) tells
more about this step:

```python
>>> import numpy as np
>>> import pynumstore as ns
>>> db = ns.Database("tour.db")
```

The seven queries
-----------------

The query language of Numstore is small. It is not SQL. This is intentional. A
Numstore database has seven operations:

1. `CREATE`
2. `GET`
3. `DELETE`
4. `INSERT`
5. `WRITE`
6. `READ`
7. `REMOVE`

`CREATE`, `GET` and `DELETE` are _variable operations_. `INSERT`, `WRITE`,
`READ` and `REMOVE` are _array operations_.

In Python, a query is a string. The method that you use depends on what the
query moves across the boundary: nothing, metadata, data in or data out:

| Method               | Queries it accepts         | Returns                        |
| -------------------- | -------------------------- | ------------------------------ |
| `db.execute(q)`      | `CREATE` `DELETE` `REMOVE` | `None`                         |
| `db.get(q)`          | Anything with a variable   | a `Var`, `None` if not exists  |
| `db.get_all()`       | takes no query             | `list[Var]`, one per variable  |
| `db.write(q, data)`  | `INSERT` `WRITE`           | `int` - elements written       |
| `db.read(q)`         | `READ` `REMOVE`            | a fresh `numpy` array          |
| `db.read_into(q, o)` | `READ` `REMOVE`            | `int` - elements read into `o` |

If you give a query to the wrong method, you get an error:

```python
>>> db.execute("read foo")
Traceback (most recent call last):
  ...
RuntimeError: Only supported exec commands are REMOVE/CREATE/DELETE
```

A string that is not a query also gives an error:

```python
>>> db.execute("not a real query")
Traceback (most recent call last):
  ...
RuntimeError: Expected a valid operation at pos: 0
```

Variable operations
-------------------

A Numstore database maps "variables" to "arrays". That is why `CREATE`, `GET`
and `DELETE` exist.

A Numstore database can look like this:

```text
"foo"  ->  [1, 2, 3, 4, 5, ...]
"bar"  ->  [1, 3, 8, 9, 10, ...]
"biz"  ->  [3, 6, 7, 8, 9, ...]
...
```

To make a variable, run `CREATE`. `create` takes a name and a type. It returns
nothing. So in the REPL, a `create` that works shows no output:

```python
>>> db.execute("create foo u32")
>>> db.execute("create bar f32")
>>> db.execute("create biz cf128")
```

To see the data about a variable, run `GET`. `db.get` returns a `Var`. A `Var`
is a frozen snapshot of the variable's metadata:

```python
>>> db.get("get foo")
Var(name='foo', type='u32', length=0, tsize=4)
```

`get` also works for any query that contains a variable. The query does not
run. So you read the variable only, not the data:

```python
>>> db.get("read foo[0:10]")
Var(name='foo', type='u32', length=0, tsize=4)
>>> db.get("remove foo[0:10]") # I don't modify foo
Var(name='foo', type='u32', length=0, tsize=4)
```

1. `name` is the name of the variable.
2. `type` is the data type of the array. See
   [The type system](../concepts/types.md).
3. `length` is the number of elements in the array.
4. `tsize` is the size in bytes of one element of the array `foo`. A `u32` is
   4 bytes.

A `Var` is a dataclass. The fields are attributes. A `Var` also has two
properties that come from numpy:

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

`var.dtype` is the numpy dtype of one element. `var.shape` is the shape of an
array that holds the whole variable. Here it is `(0,)`, because `foo` is empty.
A `Var` is a snapshot, not a live view. It shows the state at the time that you
asked.

You can run `CREATE` again with the same type. Then the variable and its data
do not change:

```python
>>> db.execute("create foo u32")
```

If the name exists and the type is different, you get an error. Numstore does
not replace the variable:

```python
>>> db.execute("create foo f64")
Traceback (most recent call last):
  ...
RuntimeError: Trying to create variable: foo but variable already exists and types are different
```

To delete a variable, run `DELETE`:

```python
>>> db.execute("delete biz")
>>> db.get("get biz")
Traceback (most recent call last):
  ...
RuntimeError: Variable: biz doesn't exist
```

### Listing every variable

`db.get_all` is `get` for the whole database. It takes no query. It returns one
`Var` for each variable. Use it to find what is in a database file that you did
not make. The order is not defined. Sort the list if you need a fixed order:

```python
>>> sorted(v.name for v in db.get_all())
['bar', 'foo']
>>> {v.name: v.type for v in sorted(db.get_all(), key=lambda v: v.name)}
{'bar': 'f32', 'foo': 'u32'}
```

Array operations
----------------

Each variable has one array. So first, make a variable:

```python
>>> db.execute("create data u32")
>>> db.get("get data")
Var(name='data', type='u32', length=0, tsize=4)
```

`INSERT` is the only way to _add_ data to an array, that is, to make the array
longer. `WRITE` does not change the size of the array. It only replaces data.
`INSERT` and `REMOVE` are the only operations that change the length.

### INSERT

To `INSERT` data, give the _offset_ where the data starts and the number of
elements. The data is the second argument of `db.write`:

```python
>>> db.write("insert data 0 5", [1, 2, 3, 4, 5])
5
>>> db.read("read data")
array([1, 2, 3, 4, 5], dtype=uint32)
```

An insert query has this form:

```text
insert <Variable Name> <Offset> <Count>
```

* _Offset_ (`0` in the example above) is the index where the data goes. A
  negative number counts back from the end of the array. `-1` is the last
  index.
* _Count_ (`5` in the example above) is the number of elements to insert. It
  is a ceiling, not a promise. See [Count is a ceiling](#count-is-a-ceiling).

The return value is the number of elements that went in.

For example, an insert at index 2 of a 6 element array "pushes" data into
index 2:

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

Here are real inserts. `data` is `[1, 2, 3, 4, 5]` from above:

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

To append, use an offset equal to the length of the array. An offset past the
end also appends, because offsets clamp:

```python
>>> v = db.get("get data")
>>> db.write(f"insert data {len(v)} 2", [10, 11])
2
>>> db.read("read data")
array([ 1,  2,  7,  8,  3,  4,  9,  5, 10, 11], dtype=uint32)
```

#### Count is a ceiling

The _Count_ in the query and the length of the data can be different. `INSERT`
pairs them. It stops when one of them ends. The return value tells you how
many elements went in:

```python
>>> db.execute("create cap u32")
>>> db.write("insert cap 0 3", [1, 2])
2
>>> db.write("insert cap 0 2", [7, 8, 9])
2
>>> db.read("read cap")
array([7, 8, 1, 2], dtype=uint32)
```

The first call asked for 3 elements, but the data had only 2. The second call
had 3 elements, but asked for only 2. So Numstore did not use the `9`.

### WRITE

`INSERT` makes the array longer. `WRITE` replaces data in its position. To
write data, give the _range_ to change:

```python
>>> db.execute("create w u32")
>>> db.write("insert w 0 6", [1, 2, 3, 4, 5, 6])
6
>>> db.write("write w[2:5]", [70, 80, 90])
3
>>> db.read("read w")
array([ 1,  2, 70, 80, 90,  6], dtype=uint32)
```

A write query has this form:

```text
write <Variable Name>[ "[" ( index
      | [ start ] ":" [ stop ] [ ":" [ step ] ]
      ) "]" ]
```

Each part of the range is optional:

* _start_ is the first index to write. The default is `0`. A negative number
  counts back from the end of the array. `-1` is the last index.
* _stop_ is the index to stop at. It is **exclusive**. The default is the
  length of the array. A negative number counts back from the end.
* _step_ is the distance between the indexes to write. The default is `1`. A
  step of `0` is an error.

If `w` is a 6 element array, these commands all cover the whole array:

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

No data moves. The array has the same length after the write:

```text
write foo[2:5]  <- [7, 8, 9]
Before:
  [1, 2, 3, 4, 5, 6]
         ^  ^  ^
        [7, 8, 9]
After:
  [1, 2, 7, 8, 9, 6]
```

Compare this with `insert foo 2 3`. The insert pushes the old data to the
right, and you get a 9 element array. To make an array longer, use `INSERT`.

A _step_ spreads the write across the array. The data is not contiguous:

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

A negative _step_ would go through the array backwards. Numstore does not
support negative steps yet. They are a planned feature:

```python
>>> db.write("write w[0:6:-2]", [7, 8, 9])
Traceback (most recent call last):
  ...
RuntimeError: stride step must be positive
```

The same check refuses a step of `0`:

```python
>>> db.write("write w[0:6:0]", [7, 8, 9])
Traceback (most recent call last):
  ...
RuntimeError: stride step must be positive
```

The range and the data can have different lengths. `WRITE` pairs them one
element at a time. It stops when one of them ends. It returns the number of
elements that it wrote. If the data ends first, the rest of the range does not
change:

```python
>>> db.write("write w", [1, 2, 3, 4, 5, 6])
6
>>> db.write("write w[:]", [70, 80])
2
>>> db.read("read w")
array([70, 80,  3,  4,  5,  6], dtype=uint32)
```

If the range ends first, Numstore does not use the rest of the data:

```python
>>> db.write("write w[0:2]", [11, 12, 13, 14, 15])
2
>>> db.read("read w")
array([11, 12,  3,  4,  5,  6], dtype=uint32)
```

An empty range is permitted. It writes nothing. A write to the range just after
the end of an array is **not** an append. It does nothing and returns `0`:

```python
>>> db.write("write w[6:]", [7, 8, 9])
0
>>> db.read("read w")
array([11, 12,  3,  4,  5,  6], dtype=uint32)
```

#### Ranges clamp

Numstore does not check ranges like Python checks a list index. It clamps
_start_ and _stop_ to the array. So a range that goes past one end covers less
than you asked for:

```python
>>> db.write("write w[0:100]", [21, 22])
2
>>> db.write("write w[-100:]", [31, 32])
2
>>> db.read("read w")
array([31, 32,  3,  4,  5,  6], dtype=uint32)
```

#### A bare index

One number in the brackets, with no colon, is a bare index. It covers exactly
one element. The colon makes a range open at the end. So `w[2]` and `w[2:]` are
different:

```python
>>> db.read("read w[2]")
array([3], dtype=uint32)
>>> db.read("read w[2:]")
array([3, 4, 5, 6], dtype=uint32)
```

`w[2]` and `w[2:3]` are the same. A bare index works in all positions where a
range works. So use it to touch one element with any of the three operations:

```python
>>> db.write("write w[2]", [70, 80, 90])
1
>>> db.read("read w")
array([31, 32, 70,  4,  5,  6], dtype=uint32)
```

The write asked to put three elements, but the range had space for one. So only
one went in. This is the pairing rule above.

A bare index counts from the end like any other index. It also clamps like any
other index. So an index past the end is an empty range, not an error:

```python
>>> db.read("read w[-1]")
array([6], dtype=uint32)
>>> db.read_into("read w[6]", np.zeros(1, dtype=np.uint32))
0
```

### READ

To `READ` data, give the _range_ to read:

```python
>>> db.execute("create r u32")
>>> db.write("insert r 0 6", [1, 2, 3, 4, 5, 6])
6
>>> db.read("read r[2:5]")
array([3, 4, 5], dtype=uint32)
```

A read query has this form:

```text
read <Variable Name>[ "[" ( index
     | [ start ] ":" [ stop ] [ ":" [ step ] ]
     ) "]" ]
```

`READ` uses ranges the same way as `WRITE`. See [WRITE](#write).

`WRITE` puts data into the range. `READ` takes data out of it. `db.read` makes a
new numpy array with the correct dtype and shape, and gives it to you.

If `r` is a 6 element array, these commands do the same thing:

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

A read never changes the array:

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

A _step_ spreads the read across the array, as it spreads a write:

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

A negative number counts back from the end:

```python
>>> db.read("read r[-3:]")
array([4, 5, 6], dtype=uint32)
```

Numstore does not support negative steps yet. So you cannot read an array
backwards:

```python
>>> db.read("read r[0:6:-2]")
Traceback (most recent call last):
  ...
RuntimeError: stride step must be positive
```

An empty range is permitted. But `db.read` has nothing to allocate for it, so it
raises an error. To read "the data that is there, possibly nothing", use
`db.read_into`. It returns `0`:

```python
>>> db.read("read r[6:]")
Traceback (most recent call last):
  ...
RuntimeError: numstore operation failed
>>> db.read_into("read r[6:]", np.zeros(3, dtype=np.uint32))
0
```

A read of a variable that has no data works. You get an empty array:

```python
>>> db.execute("create empty u32")
>>> db.read("read empty")
array([], dtype=uint32)
```

### REMOVE

To `REMOVE` data, give the _range_ to remove:

```python
>>> db.execute("create x u32")
>>> db.write("insert x 0 6", [1, 2, 3, 4, 5, 6])
6
>>> db.execute("remove x[2:5]")
>>> db.read("read x")
array([1, 2, 6], dtype=uint32)
```

A remove query has this form:

```text
remove <Variable Name>[ "[" ( index
       | [ start ] ":" [ stop ] [ ":" [ step ] ]
       ) "]" ]
```

`REMOVE` uses ranges the same way as `READ`. See [READ](#read).

`REMOVE` is the opposite of `INSERT`. It is the only operation other than
`INSERT` that changes the length of an array. Numstore takes out all of the
elements in the range. The remaining elements close the gap.

`REMOVE` is the only operation that two methods accept. `db.execute` discards
the removed data. `db.read` gives the removed data to you. So `db.read` with
`remove` is a destructive read:

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

With both methods, the length of the variable follows the data. So `get` after
a remove shows what is left:

```python
>>> db.get("get x")
Var(name='x', type='u32', length=4, tsize=4)
>>> db.execute("remove x[0]")
>>> db.get("get x")
Var(name='x', type='u32', length=3, tsize=4)
>>> db.read("read x")
array([1, 2, 6], dtype=uint32)
```

For example, a remove of the range `2:5` from a 6 element array takes out three
elements. A 3 element array stays:

```text
remove foo[2:5]
Before:
  [1, 2, 3, 4, 5, 6]
         ^  ^  ^
After:
  [1, 2, 6]
```

A _step_ makes holes in the array. It does not take out one contiguous block.
The array still closes up after the remove:

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

All parts of the range are optional. So these commands all remove all of the
data. The variable becomes empty, but it stays. You get an array of length 0:

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

An empty range is permitted. It removes nothing:

```python
>>> db.write("insert x 0 3", [1, 2, 3])
3
>>> db.execute("remove x[6:]")
>>> db.read("read x")
array([1, 2, 3], dtype=uint32)
```

Next steps
----------

* [Transactions](transactions.md) - group these queries into one unit.
* [Type sizes](type_sizes.md) - use types other than scalars.
* [Move data in and out from Python](../howto/move_data_in_python.md).
