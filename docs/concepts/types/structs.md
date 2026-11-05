# Structs

A `Struct` is a product type. That essentially means sub types are "stacked" on
top of each other. Unlike programming languages, there is no struct padding.
Numstore structs are WYSIWYG.

In the following example:

```python
>>> db.execute("create s struct { a u32, b cf128 }")
```

`s` is a variable whose elements as bytes in memory look like:

`[ 4 byte int ][    16 byte complex float    ]`

Because there's no padding, **the size of a struct is just the sum of its
members**. Here that's `4 + 16 = 20` bytes:

```python
>>> db.get("get s")
Var(name='s', type='struct { a u32, b cf128 }', length=0, tsize=20)
```

Members are laid out in the order you write them, and each member starts exactly
where the previous one ended. `a` sits at byte offset 0, `b` sits at byte
offset 4. Numpy's dtype for the same type says so too:

```python
>>> ns.to_dtype("struct { a u32, b cf128 }")
dtype([('a', '<u4'), ('b', '<c16')])
```

A struct variable reads and writes as a numpy structured array, so you can hand
it a list of tuples and pull a single field back out by name:

```python
>>> db.execute("create pt struct { x f32, y f32, z f32 }")
>>> db.write("insert pt 0 2", [(1, 2, 3), (4, 5, 6)])
2
>>> db.read("read pt")
array([(1., 2., 3.), (4., 5., 6.)],
      dtype=[('x', '<f4'), ('y', '<f4'), ('z', '<f4')])
>>> db.read("read pt")["y"]
array([2., 5.], dtype=float32)
```
