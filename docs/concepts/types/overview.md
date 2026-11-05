# Type System Overview

In all the previous examples, we created _arrays of scalars_. Numstore supports
far more than arrays of scalars, in fact, our original example was an array of
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
>>> ns.to_dtype("u32")
dtype('uint32')
>>> ns.to_dtype("u32").itemsize
4
```

## Type families at a glance

**Primitives.** The number is the size in bits.

| Family                 | Types             | Size         |
| ---------------------- | ----------------- | -------------|
| unsigned int           | `u8` to `u64`     | 1-8 bytes    |
| signed int             | `i8` to `i64`     | 1-8 bytes    |
| float                  | `f16` to `f128`   | 2-16 bytes   |
| complex float          | `cf32` to `cf256` | 4-32 bytes   |
| complex signed int     | `ci16` to `ci128` | 2-16 bytes   |
| complex unsigned int   | `cu16` to `cu128` | 2-16 bytes   |

**Composites.** Build bigger types out of smaller ones:

* **Strict array:** a fixed size, multi-dimensional array. `[3][256][256] f32`
  is three 256x256 grids of floats, like an RGB image.
* **Struct:** fields stored one after another, with no padding. The size is the
  sum of the fields. `struct { a u32, b f16 }` is 4 + 2 = 6 bytes.
* **Union:** fields that overlap, so it holds one of them at a time. The size
  is that of the largest field. `union { a u32, b f16 }` is 4 bytes.

They nest freely: `struct { a u32, b struct { c f16, d [10]f32 } }` is 4 + 2 +
40 = 46 bytes.

The full rules for sizing and nesting types are in
[the type system](#the-numstore-type-system).
