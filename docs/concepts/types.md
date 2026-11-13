The Type System
===============

The examples in the [query language tutorial](../tutorials/language_tour.md)
make _arrays of scalars_. Numstore supports much more than arrays of scalars.
For example, it can store an array of `256x256` images. Numstore has 4 main
types:

1. `Primitive`
2. `Struct`
3. `Union`
4. `Strict Array`

All Numstore types are made from these 4. Structs, unions and arrays join other
types together. At the bottom of each type is a `Primitive`. When you know the
size of each of the 4, you know the size of any Numstore type.

Each Numstore type maps to a numpy dtype. `ns.to_dtype` shows the mapping. It
does not use a database:

```python
>>> import numpy as np
>>> import pynumstore as ns
>>> db = ns.Database("types.db")
>>> ns.to_dtype("u32")
dtype('uint32')
>>> ns.to_dtype("u32").itemsize
4
```

The examples on this page are one Python session. Type them in order.

Primitives
----------

`Primitives` are the basic data types of Numstore. They are signed or unsigned,
float or integer, complex or real, and they have a number of bits.

The signed integers are:

* `i8` - a 1 byte signed integer
* `i16` - a 2 byte signed integer
* `i32` - a 4 byte signed integer
* `i64` - an 8 byte signed integer

Integers can also be unsigned:

* `u8` - a 1 byte unsigned integer
* `u16` - a 2 byte unsigned integer
* `u32` - a 4 byte unsigned integer
* `u64` - an 8 byte unsigned integer

Numstore also supports floating point numbers:

* `f16` - a 2 byte float
* `f32` - a 4 byte float
* `f64` - an 8 byte float
* `f128` - a 16 byte float

All of the types above can also be complex. A complex type is two times the
size:

* `ci16-128` - Complex signed ints
* `cu16-128` - Complex unsigned ints
* `cf32-256` - Complex floats

The number in the name of a primitive is always its size **in bits**. So the
size in bytes is `bits / 8`. A complex type stores a real part and an imaginary
part, one after the other. So its name has two times the bits of the real type
that it is made from. `cf64` is two `f32`s, so it is `64 / 8 = 8` bytes.

This is the full table:

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

Numpy has a native dtype for most of these types. For some types it does not:
the small complex integers, and `cf32`. For these, Numstore uses a structured
dtype with two fields, a real part and an imaginary part. It has the same
layout in memory:

```python
>>> ns.to_dtype("cf64")
dtype('complex64')
>>> ns.to_dtype("cu16")
dtype([('re', 'u1'), ('im', 'u1')])
```

`f128` and `cf256` need a 16 byte float. Numpy has one only where the
`long double` of the platform is 16 bytes. Numstore stores these types on all
platforms. The database does not depend on what numpy can show:

```python
>>> db.execute("create wide f128")
>>> db.get("get wide").tsize
16
```

But a read or a write goes through a dtype. Some platforms do not have a wide
`long double`, for example macOS on Apple silicon. On these platforms,
`ns.to_dtype("f128")` raises `NotImplementedError`. Each `read` or `write` of
that variable also raises it. `get` works, because it only gives sizes.

To make a variable with a primitive type, write the type after the variable
name in the `create` query:

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

`tsize` is the size of **one element** of the variable. A new variable has
`length` 0, because it has no data yet. But its `tsize` is fixed when you make
it, because the type sets it.

Structs
-------

A `Struct` is a product type. Its sub types are "stacked" one after the other.
Numstore structs have no padding. This is different from programming languages.
What you see is what you get.

In this example:

```python
>>> db.execute("create s struct { a u32, b cf128 }")
```

`s` is a variable. Each element looks like this in memory:

`[ 4 byte int ][    16 byte complex float    ]`

There is no padding. So **the size of a struct is the sum of its members**.
Here, it is `4 + 16 = 20` bytes:

```python
>>> db.get("get s")
Var(name='s', type='struct { a u32, b cf128 }', length=0, tsize=20)
```

The members are in the order that you write them. Each member starts exactly
where the member before it ends. `a` is at byte offset 0. `b` is at byte offset
4. The numpy dtype for the same type shows this:

```python
>>> ns.to_dtype("struct { a u32, b cf128 }")
dtype([('a', '<u4'), ('b', '<c16')])
```

A struct variable reads and writes as a numpy structured array. So you can give
it a list of tuples, and get one field by its name:

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

A `Union` is a sum type. Its sub types are "overlaid" on each other. Numstore
unions have no padding. This is different from programming languages. What you
see is what you get.

In this example:

```python
>>> db.execute("create u union { a u32, b cf128 }")
```

`u` is a variable. Each element looks like this in memory:

```text
a -> [ 4 byte int ]
b -> [      16 byte complex float      ]
     ^-- both members start at the same byte --^
```

All members start at the same byte. So **the size of a union is the size of its
largest member**. Here, it is `max(4, 16) = 16` bytes:

```python
>>> db.get("get u")
Var(name='u', type='union { a u32, b cf128 }', length=0, tsize=16)
```

A read of `a` uses the first 4 bytes as a `u32`. A read of `b` uses all 16
bytes as a `cf128`. It is the same memory. You select how to read it. Numpy
shows this as a structured dtype where both fields are at offset 0:

```python
>>> ns.to_dtype("union { a u32, b cf128 }")
dtype({'names': ['a', 'b'], 'formats': ['<u4', '<c16'], 'offsets': [0, 0], 'itemsize': 16})
```

Strict arrays
-------------

A `Strict Array` is an array type. Use it to store a 256x256 image in a
database.

In this example:

```python
>>> db.execute("create img [256][256] f32")
```

`img` is a 256x256 grid of `f32`s. Read the dimensions from left to right. The
outermost dimension is first. `[256][256] f32` means "an array of 256 elements.
Each element is an array of 256 `f32`."

**The size of an array is the product of its dimensions, times the size of its
element.** Each `f32` is 4 bytes. There are `256 x 256 = 65,536` of them. So one
`img` element is `65,536 x 4 = 262,144` bytes:

```python
>>> db.get("get img")
Var(name='img', type='[256][256]f32', length=0, tsize=262144)
```

A strict array variable is an array *of* arrays. So in numpy, it has more
dimensions at the end. The first axis is the length of the variable. The other
axes are the shape of the element:

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

Ranges select *elements* of the variable, not scalars. `grid[0:1]` is the first
2x3 element, not the first byte.

### Array order (C order)

Numstore stores arrays in **C order** (row-major). **The last dimension is
contiguous in memory.** The first dimension has the largest stride. When you go
through memory, the last index changes fastest.

For a small `[2][3] u8`, the memory looks like this:

```text
[ (0,0) ][ (0,1) ][ (0,2) ][ (1,0) ][ (1,1) ][ (1,2) ]
```

The second index (the column) goes from 0 to 2 before the first index (the row)
changes. That is row-major order. The bytes show it:

```python
>>> db.read("read grid[0:1]").tobytes()
b'\x00\x01\x02\x03\x04\x05'
```

In a `[R][C]` array with element size `S`, element `[i][j]` is at this byte
offset:

`offset = (i * C + j) * S`

So in the `[256][256] f32`, element `[2][3]` is `(2 * 256 + 3) * 4 = 2060` bytes
into the element. This works for any number of dimensions. Multiply each index
by the product of all of the dimensions to its right. Add the results. Then
multiply by the element size.

Combine types
-------------

A struct member can be an array. An array element can be a struct. A union can
hold an array. You can nest types to any depth. The size rules work together.
So you do not need a new rule. Use the same 4 rules again on each part:

| Type           | Size                                 |
| -------------- | ------------------------------------ |
| `Primitive`    | `bits / 8`                           |
| `Struct`       | sum of member sizes                  |
| `Union`        | max of member sizes                  |
| `Strict Array` | product of dimensions x element size |

An **array of structs**, for example a buffer of 3D points. The struct is
`4 + 4 + 4 = 12` bytes. There are 1000 of them:

```python
>>> db.execute("create points [1000] struct { x f32, y f32, z f32 }")
>>> db.get("get points").tsize
12000
```

A **struct with an array member**, for example an image with a header.
`timestamp` is 8 bytes. `pixels` is `256 x 256 x 1 = 65,536` bytes:

```python
>>> db.execute("create frame struct { timestamp u64, pixels [256][256] u8 }")
>>> db.get("get frame").tsize
65544
```

An **array of images**: 12 frames of 256x256 RGB. The struct is
`1 + 1 + 1 = 3` bytes. The array has `12 x 256 x 256 = 786,432` elements:

```python
>>> db.execute("create video [12][256][256] struct { r u8, g u8, b u8 }")
>>> db.get("get video").tsize
2359296
```

These are the same 4 rules, used on top of each other.

Numstore finds bad types when it compiles the query. This occurs before the
query touches the database:

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

To practice these rules, do the [type sizes tutorial](../tutorials/type_sizes.md).
