Move Data In and Out from Python
================================

The query language says *which* elements to touch. The Python side decides where
the bytes come from and go.

Everything below is a real Python session. Start with a database:

```python
>>> import numpy as np
>>> import pynumstore as ns
>>> db = ns.Database("data.db")
```

Get data out
------------

`db.read` allocates. It asks the variable for its dtype and shape, allocates an
array of exactly that, and hands it to you:

```python
>>> db.execute("create out u32")
>>> db.write("insert out 0 4", [1, 2, 3, 4])
4
>>> db.read("read out")
array([1, 2, 3, 4], dtype=uint32)
```

`db.read_into` doesn't allocate. You bring the buffer, it fills as much of it as
it can and returns how many elements it wrote. That makes it the tool for
reading a large variable in chunks, or for reading into memory you already own:

```python
>>> buf = np.zeros(2, dtype=np.uint32)
>>> db.read_into("read out", buf)
2
>>> buf
array([1, 2], dtype=uint32)
```

The buffer has to actually fit the variable's type. `read_into` checks the dtype
exactly - it won't convert on your behalf, because the whole point is to write
straight into your memory:

```python
>>> db.read_into("read out", np.zeros(4, dtype=np.float64))
Traceback (most recent call last):
  ...
TypeError: out has dtype float64, expected uint32
>>> db.read_into("read out", [0, 0, 0, 0])
Traceback (most recent call last):
  ...
TypeError: out must be a numpy array, not list
```

It must also be C-contiguous and writable, which rules out a non-trivial slice
of someone else's array.

Put data in
-----------

`db.write` accepts anything numpy can turn into an array. Lists, tuples, and
numpy arrays all work:

```python
>>> db.execute("create inp f64")
>>> db.write("insert inp 0 2", [1.5, 2.5])
2
>>> db.write("insert inp 2 2", (3.5, 4.5))
2
>>> db.write("insert inp 4 2", np.array([5.5, 6.5]))
2
>>> db.read("read inp")
array([1.5, 2.5, 3.5, 4.5, 5.5, 6.5])
```

When you pass a numpy array, its dtype is checked and only a same-kind cast is
allowed - so a widening cast goes through and a lossy one is refused:

```python
>>> db.write("write inp", np.array([1.5, 2.5], dtype=np.float32))
2
>>> db.execute("create ints u32")
>>> db.write("insert ints 0 2", np.array([1.5, 2.5]))
Traceback (most recent call last):
  ...
TypeError: cannot write float64 data to a uint32 variable
```

A plain list doesn't get that protection - it goes through
`np.asarray(data, dtype)`, which casts however numpy sees fit. If you care about
not silently truncating, pass an array:

```python
>>> db.write("insert ints 0 2", [1.5, 2.5])
2
>>> db.read("read ints")
array([1, 2], dtype=uint32)
```

Match the shape
---------------

For a scalar variable, the data is one-dimensional and only the element count
matters. For a strict array variable, the trailing dimensions have to match the
element type exactly, and the leading axis is the element count:

```python
>>> db.execute("create rows [3] u32")
>>> db.write("insert rows 0 2", np.arange(6, dtype=np.uint32).reshape(2, 3))
2
>>> db.write("insert rows 0 1", [[1, 2]])
Traceback (most recent call last):
  ...
ValueError: data has shape (1, 2), expected (n, 3)
>>> db.read_into("read rows", np.zeros((2, 2), dtype=np.uint32))
Traceback (most recent call last):
  ...
ValueError: out has shape (2, 2), expected (n, 3)
```

Close the database
------------------

When you're done with the database, close it. Any query after that is an error
rather than a crash:

```python
>>> db.close()
>>> db.closed
True
>>> db.read("read out")
Traceback (most recent call last):
  ...
RuntimeError: database is closed
```
