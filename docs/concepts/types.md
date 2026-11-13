The Type System
===============

In all the examples in the [query language tutorial](../tutorials/language_tour.md),
we created _arrays of scalars_. Numstore supports far more than arrays of
scalars, in fact, the [original motivation](why_numstore.md) was an array of
`256x256` images! There are 4 main types in Numstore:

1. `Primitive`
2. `Struct`
3. `Union`
4. `Strict Array`

Everything in Numstore is built out of these 4. Structs, unions, and arrays are
all ways of gluing other types together, and at the bottom of every type is a
`Primitive`. Once you know how to size these 4, you can size any type in
Numstore.

Every Numstore type maps onto a numpy dtype, and `ns.to_dtype` shows you the
mapping without touching a database:

```python
>>> import numpy as np
>>> import pynumstore as ns
>>> db = ns.Database("types.db")
>>> ns.to_dtype("u32")
dtype('uint32')
>>> ns.to_dtype("u32").itemsize
4
```

The examples on this page are one Python session, so type them in order.

Primitives
----------

`Primitives` are the core data type of Numstore. They are categorized into
signed/unsigned, floats/integers, complex/real, and number of bits.

The signed ints are:

* `i8` - a 1 byte signed integer
* `i16` - a 2 byte signed integer
* `i32` - a 4 byte signed integer
* `i64` - an 8 byte signed integer

Integers can be unsigned too:

* `u8` - a 1 byte unsigned integer
* `u16` - a 2 byte unsigned integer
* `u32` - a 4 byte unsigned integer
* `u64` - an 8 byte unsigned integer

Numstore also supports floating point numbers:

* `f16` - a 2 byte float
* `f32` - a 4 byte float
* `f64` - an 8 byte float
* `f128` - a 16 byte float

Finally, any of the above types can also be complex (effectively doubling their
size):

* `ci16-128` - Complex signed ints
* `cu16-128` - Complex unsigned ints
* `cf32-256` - Complex floats

The number in a primitive's name is always its size **in bits**, so the byte
size is just `bits / 8`. A complex type stores a real part and an imaginary part
back to back, which is why its name is double the bits of the real type it's
built from: `cf64` is two `f32`s, so `64 / 8 = 8` bytes.

Here's the full key:

| Type    | Bits | Bytes | Notes                       |
| ------- | ---- | ----- | --------------------------- |
| `i8`    | 8    | 1     | signed integer              |
| `i16`   | 16   | 2     | signed integer              |
| `i32`   | 32   | 4     | signed integer              |
| `i64`   | 64   | 8     | signed integer              |
| `u8`    | 8    | 1     | unsigned integer            |
| `u16`   | 16   | 2     | unsigned integer            |
| `u32`   | 32   | 4     | unsigned integer            |
| `u64`   | 64   | 8     | unsigned integer            |
| `f16`   | 16   | 2     | float                       |
| `f32`   | 32   | 4     | float                       |
| `f64`   | 64   | 8     | float                       |
| `f128`  | 128  | 16    | float                       |
| `ci16`  | 16   | 2     | complex `i8`  (2 x `i8`)    |
| `ci32`  | 32   | 4     | complex `i16` (2 x `i16`)   |
| `ci64`  | 64   | 8     | complex `i32` (2 x `i32`)   |
| `ci128` | 128  | 16    | complex `i64` (2 x `i64`)   |
| `cu16`  | 16   | 2     | complex `u8`  (2 x `u8`)    |
| `cu32`  | 32   | 4     | complex `u16` (2 x `u16`)   |
| `cu64`  | 64   | 8     | complex `u32` (2 x `u32`)   |
| `cu128` | 128  | 16    | complex `u64` (2 x `u64`)   |
| `cf32`  | 32   | 4     | complex `f16` (2 x `f16`)   |
| `cf64`  | 64   | 8     | complex `f32` (2 x `f32`)   |
| `cf128` | 128  | 16    | complex `f64` (2 x `f64`)   |
| `cf256` | 256  | 32    | complex `f128` (2 x `f128`) |

Numpy has a native dtype for most of these. Where it doesn't - the small complex
integers, and `cf32` - Numstore maps the type to a two-field structured dtype
with a real and an imaginary part, which has the same layout in memory:

```python
>>> ns.to_dtype("cf64")
dtype('complex64')
>>> ns.to_dtype("cu16")
dtype([('re', 'u1'), ('im', 'u1')])
```

`f128` and `cf256` need a 16 byte float, which numpy only has where the
platform's `long double` is 16 bytes wide. Numstore stores them anywhere - the
database doesn't care what numpy can represent:

```python
>>> db.execute("create wide f128")
>>> db.get("get wide").tsize
16
```

Reading or writing one, though, has to go through a dtype, so on a platform
without a wide long double (macOS on Apple silicon, for instance)
`ns.to_dtype("f128")` raises `NotImplementedError` and so does any `read` or
`write` of that variable. `get` keeps working, because it only reports sizes.

To create a primitive type, you specify the name right after the variable name
in your `create` query:

```python
>>> db.execute("create p1 u32")
>>> db.execute("create p2 ci128")
>>> db.execute("create p3 cu64")
```

The size of each element is the size of the primitive:

```python
>>> db.get("get p1")
Var(name='p1', type='u32', length=0, tsize=4)
>>> db.get("get p2")
Var(name='p2', type='ci128', length=0, tsize=16)
>>> db.get("get p3")
Var(name='p3', type='cu64', length=0, tsize=8)
```

`tsize` is the size of **one element** of the variable. A fresh variable has
`length` 0 - you haven't pushed any data in yet - but its `tsize` is locked in
the moment you create it, because that's fixed by the type.

Structs
-------

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

Unions
------

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

Strict arrays
-------------

A `Strict Array` is an array type. This is how we'd store our 256x256 image in
our database.

In the following example:

```python
>>> db.execute("create img [256][256] f32")
```

`img` is a 256x256 grid of `f32`s - exactly the image from
[the original example](why_numstore.md). You read the dimensions left to right,
outermost first: `[256][256] f32` means "a 256-element array, where each element
is a 256-element array of `f32`."

**The size of an array is the product of its dimensions times the size of its
element.** Each `f32` is 4 bytes and there are `256 x 256 = 65,536` of them, so
one `img` element is `65,536 x 4 = 262,144` bytes:

```python
>>> db.get("get img")
Var(name='img', type='[256][256]f32', length=0, tsize=262144)
```

A strict array variable is an array *of* arrays, so in numpy it picks up extra
trailing dimensions. The leading axis is the variable's length; the trailing
axes are the element's shape:

```python
>>> db.execute("create grid [2][3] u8")
>>> db.write("insert grid 0 2", np.arange(12, dtype=np.uint8).reshape(2, 2, 3))
2
>>> db.read("read grid").shape
(2, 2, 3)
>>> db.read("read grid[0:1]")
array([[[0, 1, 2],
        [3, 4, 5]]], dtype=uint8)
```

Ranges still index *elements* of the variable, not scalars - `grid[0:1]` is the
first 2x3 element, not the first byte.

### Array order (C order)

Numstore stores arrays in **C order** (row-major), which means **the last
dimension is contiguous in memory** and the first dimension has the largest
stride. The last index varies fastest as you walk through memory.

For a small `[2][3] u8`, memory looks like:

```text
[ (0,0) ][ (0,1) ][ (0,2) ][ (1,0) ][ (1,1) ][ (1,2) ]
```

Notice the second index (the column) races from 0->2 before the first index
(the row) ticks over. That's row-major. You can see it in the bytes:

```python
>>> db.read("read grid[0:1]").tobytes()
b'\x00\x01\x02\x03\x04\x05'
```

To find where element `[i][j]` lives in a `[R][C]` array of element size `S`,
the byte offset is:

`offset = (i * C + j) * S`

So in our `[256][256] f32`, element `[2][3]` sits at `(2 * 256 + 3) * 4 = 2060`
bytes into the element. This generalizes to any number of dimensions: multiply
each index by the product of all the dimensions to its right, sum them up, and
multiply by the element size.

Combine types
-------------

A struct member can be an array, an array element can be a struct, a union can
hold an array, and so on - as deep as you like. The sizing rules compose, so you
never need a new rule, you just apply the same 4 recursively:

| Type           | Size                                 |
| -------------- | ------------------------------------ |
| `Primitive`    | `bits / 8`                           |
| `Struct`       | sum of member sizes                  |
| `Union`        | max of member sizes                  |
| `Strict Array` | product of dimensions x element size |

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

Want to check your understanding? Try the [practice problems](../tutorials/type_sizes.md).
