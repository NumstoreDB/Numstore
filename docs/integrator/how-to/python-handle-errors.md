# Python: Handle Errors

Numstore reports failures as standard Python exceptions. There is no custom
exception hierarchy to import.

| Exception              | When                                                     |
|------------------------|----------------------------------------------------------|
| `RuntimeError`         | the database or the query layer refused: a missing variable, a query handed to the wrong method, a syntax error, a closed database, plan or transaction |
| `TypeError`            | the data or buffer has the wrong dtype or the wrong Python type |
| `ValueError`           | the shape doesn't fit, a type is malformed, or a transaction is from another database |
| `NotImplementedError`  | the type has no numpy representation on this platform (`f128`, `cf256`) |

The messages are written to be read, so logging the exception is usually enough:

```python
>>> try:
...     db.write("insert counts 0 1", np.array([3.5]))
... except TypeError as exc:
...     print(f"refused: {exc}")
refused: cannot write float64 data to a uint32 variable
```

Because a failed query is rolled back, catching an error inside a transaction
leaves the transaction usable - and if you let the error escape the `with`
block, the whole transaction is rolled back:

```python
>>> with db.begin() as txn:
...     try:
...         txn.write("insert counts 0 1", np.array([3.5]))
...     except TypeError:
...         txn.write("insert counts 0 1", np.array([3], dtype=np.uint32))
1
>>> db.read("read counts")
array([3, 1, 2], dtype=uint32)
```
