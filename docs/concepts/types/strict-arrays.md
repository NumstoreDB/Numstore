# Strict Arrays

A `Strict Array` is an array type. This is how we'd store our 256x256 image in
our database.

In the following example:

```python
>>> db.execute("create img [256][256] f32")
```

`img` is a 256x256 grid of `f32`s - exactly the image from our very first
example. You read the dimensions left to right, outermost first: `[256][256]
f32` means "a 256-element array, where each element is a 256-element array of
`f32`."

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

## Array ordering (C order)

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
