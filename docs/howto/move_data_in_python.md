Move Data In and Out from Python
================================

The query selects *which* elements to touch. Python selects where the bytes
come from and where they go.

Each example is a real Python session. Start with a database:

```python
>>> import numpy as np
>>> import pynumstore as ns
>>> db = ns.Database("data.db")
```

Get data out
------------

`db.read` allocates. It asks the variable for its dtype and shape. It makes an
array of exactly that, and gives it to you:

```python
>>> db.execute("create out u32")
>>> db.write("insert out 0 4", [1, 2, 3, 4])
4
>>> db.read("read out")
array([1, 2, 3, 4], dtype=uint32)
```

`db.read_into` does not allocate. You give the buffer. It fills as much of the
buffer as it can. It returns the number of elements that it wrote. Use it to
read a large variable in chunks, or to read into memory that you own:

```python
>>> buf = np.zeros(2, dtype=np.uint32)
>>> db.read_into("read out", buf)
2
>>> buf
array([1, 2], dtype=uint32)
```

The buffer must match the type of the variable. `read_into` checks the dtype
exactly. It does not convert, because its purpose is to write directly into
your memory:

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

The buffer must also be C-contiguous and writable. So you cannot use most
slices of another array.

Put data in
-----------

`db.write` accepts all data that numpy can change into an array. Lists, tuples
and numpy arrays all work:

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

When you give a numpy array, Numstore checks its dtype. It permits only a cast
of the same kind. So a cast that makes the type wider works. A cast that loses
data fails:

```python
>>> db.write("write inp", np.array([1.5, 2.5], dtype=np.float32))
2
>>> db.execute("create ints u32")
>>> db.write("insert ints 0 2", np.array([1.5, 2.5]))
Traceback (most recent call last):
  ...
TypeError: cannot write float64 data to a uint32 variable
```

A list does not get this check. It goes through `np.asarray(data, dtype)`, and
numpy casts it as numpy decides. To prevent silent truncation, give an array:

```python
>>> db.write("insert ints 0 2", [1.5, 2.5])
2
>>> db.read("read ints")
array([1, 2], dtype=uint32)
```

Match the shape
---------------

For a scalar variable, the data has one dimension. Only the number of elements
is important. For a strict array variable, the last dimensions of the data must
match the element type exactly. The first axis is the number of elements:

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

When you are done, close the database. A query after that gives an error, not
a crash:

```python
>>> db.close()
>>> db.closed
True
>>> db.read("read out")
Traceback (most recent call last):
  ...
RuntimeError: database is closed
```
