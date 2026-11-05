# Write

While `INSERT` increases the array length, `WRITE` simply overwrites data in
place. To write data, you specify the _range_ you wish to modify:

```python
>>> db.execute("create w u32")
>>> db.write("insert w 0 6", [1, 2, 3, 4, 5, 6])
6
>>> db.write("write w[2:5]", [70, 80, 90])
3
>>> db.read("read w")
array([ 1,  2, 70, 80, 90,  6], dtype=uint32)
```

The general form of a write command looks like this:

```text
write <Variable Name>[ "[" ( index
      | [ start ] ":" [ stop ] [ ":" [ step ] ]
      ) "]" ]
```

Every part of the range is optional:

* _start_ is the first index written to. If it isn't included, it defaults to
  `0`. Negative numbers count back from the end of the array, -1 being the last
  index.
* _stop_ is the index to stop at, **exclusive**. If it isn't included, it
  defaults to the length of the array. Negative numbers count back from the
  end.
* _step_ is the distance between written indexes. If it isn't included, it
  defaults to `1`. A step of `0` is an error.

Assuming `w` is a 6 element array, the following commands all cover the whole
thing:

```python
>>> db.write("write w[0:6:1]", [1, 2, 3, 4, 5, 6])
6
>>> db.write("write w[0:]", [1, 2, 3, 4, 5, 6])
6
>>> db.write("write w[:]", [1, 2, 3, 4, 5, 6])
6
>>> db.write("write w", [1, 2, 3, 4, 5, 6])
6
```

Nothing moves, and the array is the same length afterwards:

```text
write foo[2:5]  <- [7, 8, 9]
Before:
  [1, 2, 3, 4, 5, 6]
         ^  ^  ^
        [7, 8, 9]
After:
  [1, 2, 7, 8, 9, 6]
```

Compare that to `insert foo 2 3`, which pushes the old data out of the way and
leaves you with a 9 element array. If you want to grow an array, you want
`INSERT`.

A _step_ spreads the write out over the array instead of laying it down
contiguously:

```text
write foo[0:6:2]  <- [7, 8, 9]
Before:
  [1, 2, 3, 4, 5, 6]
   ^     ^     ^
   7     8     9
After:
  [7, 2, 8, 4, 9, 6]
```

```python
>>> db.write("write w[0:6:2]", [7, 8, 9])
3
>>> db.read("read w")
array([7, 2, 8, 4, 9, 6], dtype=uint32)
```

A negative _step_ would walk the array backwards. Negative steps aren't
supported yet but are a planned feature:

```python
>>> db.write("write w[0:6:-2]", [7, 8, 9])
Traceback (most recent call last):
  ...
RuntimeError: stride step must be positive
```

A step of `0` is rejected by the same check:

```python
>>> db.write("write w[0:6:0]", [7, 8, 9])
Traceback (most recent call last):
  ...
RuntimeError: stride step must be positive
```

The range and the data don't have to agree on length. `WRITE` pairs them up one
element at a time and stops as soon as either one runs out, and returns how many
elements it wrote. If the data runs out first, the rest of the range is left
exactly as it was:

```python
>>> db.write("write w", [1, 2, 3, 4, 5, 6])
6
>>> db.write("write w[:]", [70, 80])
2
>>> db.read("read w")
array([70, 80,  3,  4,  5,  6], dtype=uint32)
```

And if the range runs out first, the remaining data is simply not consumed:

```python
>>> db.write("write w[0:2]", [11, 12, 13, 14, 15])
2
>>> db.read("read w")
array([11, 12,  3,  4,  5,  6], dtype=uint32)
```

An empty range is legal and writes nothing. In particular, writing to the range
just past the end of an array is **not** an `append`, it's a no-op that returns
`0`:

```python
>>> db.write("write w[6:]", [7, 8, 9])
0
>>> db.read("read w")
array([11, 12,  3,  4,  5,  6], dtype=uint32)
```

## Ranges clamp

Ranges aren't bounds-checked the way an index into a Python list is. _start_ and
_stop_ are clamped to the array, so a range that hangs off either end just
covers less than you asked for:

```python
>>> db.write("write w[0:100]", [21, 22])
2
>>> db.write("write w[-100:]", [31, 32])
2
>>> db.read("read w")
array([31, 32,  3,  4,  5,  6], dtype=uint32)
```

## A bare index

A single number in the brackets, with no colon, is a bare index: it covers
exactly one element. The colon is what makes a range open ended, so `w[2]` and
`w[2:]` are different things:

```python
>>> db.read("read w[2]")
array([3], dtype=uint32)
>>> db.read("read w[2:]")
array([3, 4, 5, 6], dtype=uint32)
```

`w[2]` and `w[2:3]` are the same thing. A bare index works wherever a range
does, so it's the way to touch a single element with any of the three
operations:

```python
>>> db.write("write w[2]", [70, 80, 90])
1
>>> db.read("read w")
array([31, 32, 70,  4,  5,  6], dtype=uint32)
```

The write asked to lay down three elements but the range only had room for one,
so only one landed - see the pairing rule above.

A bare index counts from the end like any other, and clamps like any other, so
an index past the end is an empty range rather than an error:

```python
>>> db.read("read w[-1]")
array([6], dtype=uint32)
>>> db.read_into("read w[6]", np.zeros(1, dtype=np.uint32))
0
```
