# Combining Types

A struct member can be an array, an array element can be a struct, a union can
hold an array, and so on - as deep as you like. The sizing rules compose, so you
never need a new rule, you just apply the same 4 recursively:

| Type          | Size                                             |
|---------------|--------------------------------------------------|
| `Primitive`   | `bits / 8`                                       |
| `Struct`      | sum of member sizes                              |
| `Union`       | max of member sizes                              |
| `Strict Array`| product of dimensions x element size             |

An **array of structs** - say a buffer of 3D points. The struct is
`4 + 4 + 4 = 12` bytes, and there are 1000 of them:

```python
>>> db.execute("create points [1000] struct { x f32, y f32, z f32 }")
>>> db.get("get points").tsize
12000
```

A **struct with an array member** - an image with a header. `timestamp` is 8
bytes, `pixels` is `256 x 256 x 1 = 65,536` bytes:

```python
>>> db.execute("create frame struct { timestamp u64, pixels [256][256] u8 }")
>>> db.get("get frame").tsize
65544
```

And our original **array of images**, now fully spelled out - 12 frames of
256x256 RGB. The struct is `1 + 1 + 1 = 3` bytes and the array is
`12 x 256 x 256 = 786,432` elements:

```python
>>> db.execute("create video [12][256][256] struct { r u8, g u8, b u8 }")
>>> db.get("get video").tsize
2359296
```

Same 4 rules, just stacked.

Bad types are caught when the query is compiled, before anything touches the
database:

```python
>>> db.execute("create bad q99")
Traceback (most recent call last):
  ...
RuntimeError: Expected type at position 2, got token type TT_IDENTIFIER
>>> ns.to_dtype("struct { i u8, i u32 }")
Traceback (most recent call last):
  ...
ValueError: duplicate key: i
```
