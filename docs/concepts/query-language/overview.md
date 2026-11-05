# Query Language Overview

To get a better glimpse of what Numstore can do, it helps to understand the
Numstore language, which is very self-contained and simple.

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

| Method               | Queries it accepts        | Returns                         |
|----------------------|---------------------------|---------------------------------|
| `db.execute(q)`      | `CREATE` `DELETE` `REMOVE`| `None`                          |
| `db.get(q)`          | Anything with a variable  | a `Var` (metadata)              |
| `db.get_all()`       | takes no query            | `list[Var]`, one per variable   |
| `db.write(q, data)`  | `INSERT` `WRITE`          | `int` - elements written        |
| `db.read(q)`         | `READ` `REMOVE`           | a fresh `numpy` array           |
| `db.read_into(q, o)` | `READ` `REMOVE`           | `int` - elements read into `o`  |

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

## Array operations

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
