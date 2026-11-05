# Python: Read into a Preallocated Array

`read_into(query, out) -> int` doesn't allocate. You bring the buffer, it fills
as much as it can and returns how many elements it wrote. That makes it the tool
for reading a large variable in chunks, or for reading into memory you already
own:

```python
>>> buf = np.zeros(2, dtype=np.float64)
>>> db.read_into("read prices", buf)
2
>>> buf
array([3.5, 4.5])
```

The buffer must match the variable's dtype exactly, be C-contiguous, and be
writable - no conversion happens, because the point is to write straight into
your memory:

```python
>>> db.read_into("read prices", np.zeros(4, dtype=np.uint32))
Traceback (most recent call last):
  ...
TypeError: out has dtype uint32, expected float64
>>> db.read_into("read prices", [0, 0, 0, 0])
Traceback (most recent call last):
  ...
TypeError: out must be a numpy array, not list
>>> db.read_into("read prices", np.zeros(8, dtype=np.float64)[::2])
Traceback (most recent call last):
  ...
ValueError: out must be C-contiguous and writable
```

Reading a whole variable in fixed size chunks:

```python
>>> db.execute("create stream u32")
>>> db.write("insert stream 0 10", np.arange(10, dtype=np.uint32))
10
>>> chunk = np.zeros(4, dtype=np.uint32)
>>> total, start = 0, 0
>>> while True:
...     n = db.read_into(f"read stream[{start}:{start + 4}]", chunk)
...     if n == 0:
...         break
...     total += n
...     start += n
>>> total
10
```
