# Primitive Types

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

| Type    | Bits | Bytes | Notes                         |
|---------|------|-------|-------------------------------|
| `i8`    | 8    | 1     | signed integer                |
| `i16`   | 16   | 2     | signed integer                |
| `i32`   | 32   | 4     | signed integer                |
| `i64`   | 64   | 8     | signed integer                |
| `u8`    | 8    | 1     | unsigned integer              |
| `u16`   | 16   | 2     | unsigned integer              |
| `u32`   | 32   | 4     | unsigned integer              |
| `u64`   | 64   | 8     | unsigned integer              |
| `f16`   | 16   | 2     | float                         |
| `f32`   | 32   | 4     | float                         |
| `f64`   | 64   | 8     | float                         |
| `f128`  | 128  | 16    | float                         |
| `ci16`  | 16   | 2     | complex `i8`  (2 x `i8`)      |
| `ci32`  | 32   | 4     | complex `i16` (2 x `i16`)     |
| `ci64`  | 64   | 8     | complex `i32` (2 x `i32`)     |
| `ci128` | 128  | 16    | complex `i64` (2 x `i64`)     |
| `cu16`  | 16   | 2     | complex `u8`  (2 x `u8`)      |
| `cu32`  | 32   | 4     | complex `u16` (2 x `u16`)     |
| `cu64`  | 64   | 8     | complex `u32` (2 x `u32`)     |
| `cu128` | 128  | 16    | complex `u64` (2 x `u64`)     |
| `cf32`  | 32   | 4     | complex `f16` (2 x `f16`)     |
| `cf64`  | 64   | 8     | complex `f32` (2 x `f32`)     |
| `cf128` | 128  | 16    | complex `f64` (2 x `f64`)     |
| `cf256` | 256  | 32    | complex `f128` (2 x `f128`)   |

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
