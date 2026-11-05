# Insert

To `INSERT` data, you specify what _offset_ to insert at and how many elements
you're inserting. The data itself goes to `db.write` as the second argument:

```python
>>> db.write("insert data 0 5", [1, 2, 3, 4, 5])
5
>>> db.read("read data")
array([1, 2, 3, 4, 5], dtype=uint32)
```

The general form of an insert command looks like this:

```text
insert <Variable Name> <Offset> <Count>
```

* _Offset_ (`0` in the example above) is where to insert the data into the
  array. Negative numbers count back from the end of the array, -1 being the
  last index.
* _Count_ (`5` in the example above) is how many elements to insert. It's a
  ceiling, not a promise - see [Count is a ceiling](#count-is-a-ceiling).

The return value is the number of elements actually inserted.

For example, if we insert into a 6 element array at index 2, it's like we're
"pushing" data into index 2:

```text
insert foo 2 3   <- [7, 8, 9]

Before:
  [1, 2, 3, 4, 5, 6]
         ^
     [7, 8, 9]

After:
  [1, 2, 7, 8, 9, 3, 4, 5, 6]
```

```text
insert foo 0 3   <- [7, 8, 9]

Before:
  [1, 2, 3, 4, 5, 6]
   ^
[7, 8, 9]

After:
  [7, 8, 9, 1, 2, 3, 4, 5, 6]
```

```text
insert foo -1 3  <- [7, 8, 9]

Before:
  [1, 2, 3, 4, 5, 6]
                  ^
              [7, 8, 9]

After:
  [1, 2, 3, 4, 5, 7, 8, 9, 6]
```

And here it is for real. `data` is `[1, 2, 3, 4, 5]` from above:

```python
>>> db.write("insert data 2 2", [7, 8])
2
>>> db.read("read data")
array([1, 2, 7, 8, 3, 4, 5], dtype=uint32)
>>> db.write("insert data -1 1", [9])
1
>>> db.read("read data")
array([1, 2, 7, 8, 3, 4, 9, 5], dtype=uint32)
```

A true `append` is an offset equal to the length of the array - or any offset
past it, because offsets clamp:

```python
>>> v = db.get("get data")
>>> db.write(f"insert data {len(v)} 2", [10, 11])
2
>>> db.read("read data")
array([ 1,  2,  7,  8,  3,  4,  9,  5, 10, 11], dtype=uint32)
```

## Count is a ceiling

The _Count_ in the query and the length of the data you pass don't have to
agree. `INSERT` pairs them up and stops as soon as either one runs out, and the
return value tells you how many elements actually landed:

```python
>>> db.execute("create cap u32")
>>> db.write("insert cap 0 3", [1, 2])
2
>>> db.write("insert cap 0 2", [7, 8, 9])
2
>>> db.read("read cap")
array([7, 8, 1, 2], dtype=uint32)
```

The first call asked for 3 but only had 2 elements to give. The second had 3
elements but only asked for 2, so `9` was never consumed.
