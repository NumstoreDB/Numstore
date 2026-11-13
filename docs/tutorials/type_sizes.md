Type Sizes
==========

Once you know how to size the 4 basic types, you can size any type in Numstore.
If you haven't yet, read [the type system](../concepts/types.md) first - it
walks through each rule.

The size rules
--------------

| Type           | Size                                 |
| -------------- | ------------------------------------ |
| `Primitive`    | `bits / 8`                           |
| `Struct`       | sum of member sizes                  |
| `Union`        | max of member sizes                  |
| `Strict Array` | product of dimensions x element size |

Practice problems
-----------------

`ns.to_dtype(...).itemsize` will check your answers.

1. What is the element size of `struct { a u16, b i64, c f32 }`?
2. What is the element size of `union { a u16, b i64, c f32 }`?
3. What is the element size of `[10][20] u32`?
4. What is the element size of `[100] struct { x f32, y f32, z f32 }`?
5. What is the element size of `struct { id u64, samples [8] cf64 }`?
6. What is the element size of `[12][256][256] struct { r u8, g u8, b u8 }`?
7. In a `[4][4] f32`, what byte offset is element `[2][3]`?
8. What is the element size of
   `struct { tag u8, payload union { a f64, b [2] u32 } }`?

Answers
-------

1. `2 + 8 + 4 = 14` bytes.
2. `max(2, 8, 4) = 8` bytes.
3. `10 x 20 x 4 = 800` bytes.
4. struct is `12`, so `100 x 12 = 1,200` bytes.
5. `cf64` is 8 bytes -> `samples` is `8 x 8 = 64`, plus `id` at 8 -> `72` bytes.
6. struct is `3`, array is `12 x 256 x 256 = 786,432` -> `2,359,296` bytes.
7. `(2 x 4 + 3) x 4 = 11 x 4 = 44` bytes.
8. `union` is `max(8, 2 x 4) = 8`, plus `tag` at 1 -> `9` bytes.

Check the answers
-----------------

```python
>>> import pynumstore as ns
>>> ns.to_dtype("struct { a u16, b i64, c f32 }").itemsize
14
>>> ns.to_dtype("union { a u16, b i64, c f32 }").itemsize
8
>>> ns.to_dtype("[10][20] u32").itemsize
800
>>> ns.to_dtype("[100] struct { x f32, y f32, z f32 }").itemsize
1200
>>> ns.to_dtype("struct { id u64, samples [8] cf64 }").itemsize
72
>>> ns.to_dtype("[12][256][256] struct { r u8, g u8, b u8 }").itemsize
2359296
>>> ns.to_dtype("struct { tag u8, payload union { a f64, b [2] u32 } }").itemsize
9
```
