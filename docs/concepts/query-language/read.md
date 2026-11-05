# Read

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

`READ` has the same range semantics as `WRITE`. See [WRITE](write.md).

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
