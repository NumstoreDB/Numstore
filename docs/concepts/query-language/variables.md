# Variable Operations

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
   [The Numstore Type System](../types/overview.md)
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

## Listing every variable

`db.get_all` is `get` for the whole database: it takes no query and hands back
one `Var` per variable. It's how you find out what's in a database file you
didn't create. The order isn't specified, so sort if you need a stable list:

```python
>>> sorted(v.name for v in db.get_all())
['bar', 'foo']
>>> {v.name: v.type for v in sorted(db.get_all(), key=lambda v: v.name)}
{'bar': 'f32', 'foo': 'u32'}
```
