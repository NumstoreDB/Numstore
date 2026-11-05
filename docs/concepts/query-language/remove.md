# Remove

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

`REMOVE` has the same range semantics as `READ`. See [READ](read.md).

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
