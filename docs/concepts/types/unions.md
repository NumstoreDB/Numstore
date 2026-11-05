# Unions

A `Union` is a sum type. That essentially means sub types are "overlayed" on top
of each other. Unlike programming languages, there is no union padding. Numstore
unions are WYSIWYG.

In the following example:

```python
>>> db.execute("create u union { a u32, b cf128 }")
```

`u` is a variable whose elements as bytes in memory look like:

```text
a -> [ 4 byte int ]
b -> [      16 byte complex float      ]
     ^-- both members start at the same byte --^
```

Every member shares the same starting byte, so **the size of a union is the
size of its largest member** - here `max(4, 16) = 16` bytes:

```python
>>> db.get("get u")
Var(name='u', type='union { a u32, b cf128 }', length=0, tsize=16)
```

Reading `a` interprets the first 4 bytes as a `u32`; reading `b` interprets all
16 bytes as a `cf128`. It's the same memory - you just choose how to read it.
Numpy spells that as a structured dtype whose fields both sit at offset 0:

```python
>>> ns.to_dtype("union { a u32, b cf128 }")
dtype({'names': ['a', 'b'], 'formats': ['<u4', '<c16'], 'offsets': [0, 0], 'itemsize': 16})
```
