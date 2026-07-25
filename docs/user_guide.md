Table of Contents
=================

* [What is Numstore?](#1-introduction)
    * [How is it different than other databases?](#how-is-it-different-than-other-databases)
* [A tour of the language](#a-tour-of-the-language)
  * [Variable Operations](#variable-operations)
  * [Array Operations](#array-operations)
    * [Insert](#INSERT)
    * [Write](#WRITE)
    * [Read](#READ)
    * [Remove](#REMOVE)
  * [The Numstore Type System](#the-numstore-type-system)
  * [Variable References](#variable-references)
  * [Variable Constructions](#variable-constructions)

What is Numstore?
=================

Numstore is a database for arrays. It is an ACID database meant to store contiguous 
arrays of numerical values. 

How is it different than other databases?
-----------------------------------------

SQL is great. It's versatile and in record oriented computing, SQL solves the data 
storage problem very well. Numstore doesn't compete with SQL. Numstore is meant to 
store a different shape of data: _arrays_.

In traditional relational databases, data is organized into _Tables_ (a singular 
_type_) which are further organized into _columns_. Columns are named quantities.
Each column has a name, and to access certain data you specify what table(s) you 
want and what column(s) you want. This is tabular data. 

Would you ever store a 256x256 pixel image in a Relational database? Let's think of 
ways to do this:

1. Store each pixel in it's own column - up to pixel 65536. This breaks down easily 
   with column limits. Postgres limits columns to 1600. For small arrays this is 
   a tempting idea. A 5 element long array of floats could just be stored as 5 columns 
   but in general, arrays are things were the "names" of columns are implicitly the 
   index where they reside.

2. Storing a reference to the file system data. This is probably the most common 
   approach to storing high dimensionality data in databases. You store the location 
   of the file on disk (e.g. 'file:///foo/bar/biz/image.png') or where it is on 
   the internet (e.g. 'https://amazonaws.com/foo/bar/biz/image.png'). This is nice,
   but now you have two sources of truth. What happens if you move the image and don't 
   update the database? Fundamentally, you're removing all the ACID properties of the 
   database in favor of just storing the image in an unreliable file system.

3. Store the image data as some internal column type. We could store the image pixels 
   as a BYTEA or something similar. This solves some of the problems of (2), but it 
   doesn't always make sense to store large files in a database column. This often bloats 
   database size and degrades memory consumption because fundamentally, relational databases 
   weren't built for that type of data.

A fundamental limitation of relational databases is that _they don't support packed 
high dimensional data natively_. There are file formats out there:

1. Hdf5
2. Parquette
3. Zarr-Python

But each one makes it very clear that they aren't databases because: 

1. Databases should support concurrent writers / readers and logical isolation 
   meaning two threads don't know about each other's work on the same data but 
   keep the database in a consistent state.

2. Databases should support transactions - sure Hdf5 is atomic, but you can't 
   do multiple things to an Hdf5 file atomically.

Numstore is the database for this type of data.

A tour of the language
======================

To get a better glimpse of what Numstore can do, it helps to understand the Numstore language, 
which is very self contained and simple.

Numstore's query language is not SQL. This was a very intentional decision. A Numstore database 
has seven fundamental operations:

1. `CREATE`
2. `GET`
3. `DELETE`
4. `INSERT`
5. `WRITE`
6. `READ`
7. `REMOVE`

You could put `CREATE`, `GET` and `DELETE` in their own category of _variable operations_ 
and `INSERT`, `WRITE`, `READ` and `REMOVE` in their own category of _array operations_. 

Variable Operations 
-------------------

`CREATE` `GET` and `DELETE` exist because a Numstore database 
is really just a map of "variables" to "arrays".

A Numstore database may look like this:
```
"foo"  ->  [1, 2, 3, 4, 5, ...]
"bar"  ->  [1, 3, 8, 9, 10, ...]
"biz"  ->  [3, 6, 7, 8, 9, ...]
...
```

To create new variables you execute the `CREATE` operation:
```
numstore> create foo u32;
{ "Status" : "Ok" }

numstore> create bar f32;
{ "Status" : "Ok" }

numstore> create biz cf64;
{ "Status" : "Ok" }
...
```

You can get a variable's information by invoking the `GET` operation:

```
get foo;
{
    "Status" : "Ok",
    "Name"   : "foo",
    "DType"  : "u32",
    "DSize"  : 4,
    "Nelems" : 1296,
    "Bytes"  : 5184,
    "Root"   : 12398312
}
```

1. `Name` is the variable name 
2. `DType` is the data type of the array - explained further in TODO TYPE LINK
3. `DSize` is the size of each element inside the array `foo` (a `u32` is 4 bytes)
4. `Nelems` is the number of elements in the array 
5. `Bytes` is the total size of the array in bytes.
6. `Root` is the page number that the root of the variable's array begins

You can delete a variable by invoking the `DELETE` operation:

```
numstore> delete foo;
{ "Status" : "Ok" }

numstore> get foo;
{ 
    "Status" : "Error", 
    "Message" : "No variable with name 'foo' exists"
}
```

Array Operations 
----------------

Each variable has one array. So first, you need to create a variable:

```
numstore> create foo u32;
{ "Status" : "Ok" }

numstore> get foo;
{
    "Status" : "Ok",
    "Name"   : "foo",
    "DType"  : "u32",
    "DSize"  : 4,
    "Nelems" : 0,
    "Bytes"  : 0,
    "Root"   : null,
}
```

The only way to _add_ data to an array (e.g. increase the array size) is 
to `INSERT` data into the array. `WRITE` doesn't increase the array size, 
it only overwrites data. `INSERT` and `REMOVE` are the only length modifying 
operations. 

### INSERT

To `INSERT` data, you specify what _offset_ to insert the data into and 
optionally the data. 

```
numstore> insert foo[0] [ 1, 2, 3, 4, 5 ];
{ "Status" : "Ok" }
```

The general form of an insert command looks like this: 
```
insert <Variable Name>[ "[" Offset "]" ] [ data ];
```

* _Offset_ (`0` in the example above) is where to insert the data into the array.
  If it isn't included, it appends to the end of the array. 
  Negative numbers count back from the end of the array, -1 being the last index
* _data_ is an optional explicit data source. See TODO DATA SECTION for more information 
  on how numstore manages data input and output streams.

Assuming `foo` is an empty array to begin with, the following commands 
do the same thing:

```
numstore> insert foo[0] [ 1, 2, 3, 4, 5];
{ "Status" : "Ok" }

numstore> insert foo [ 1, 2, 3, 4, 5];
{ "Status" : "Ok" }
```

For example, if we insert an array into a 6 element array at index 2, it's like 
we're "pushing" data into index 2:

```
insert foo[2] [7, 8, 9]

Before:
  [1, 2, 3, 4, 5, 6]
         ^
     [7, 8, 9]

After:
  [1, 2, 7, 8, 9, 3, 4, 5, 6]
```

```
insert foo[0] [7, 8, 9]

Before:
  [1, 2, 3, 4, 5, 6]
   ^
[7, 8, 9]

After:
  [7, 8, 9, 1, 2, 3, 4, 5, 6]
```

```
insert foo[-1] [7, 8, 9]

Before:
  [1, 2, 3, 4, 5, 6]
                  ^
              [7, 8, 9]

After:
  [1, 2, 3, 4, 5, 7, 8, 9, 6]
```

A true `append` operation is the absence of an _offset_:

```
insert foo [7, 8, 9]

Before:
  [1, 2, 3, 4, 5, 6]
                   ^
               [7, 8, 9]

After:
  [1, 2, 3, 4, 5, 6, 7, 8, 9]
```

`INSERT` is strongly indexed. You can't insert data using invalid indexes:

```
numstore> create foo;
{ "Status" : "Ok" }

numstore> insert foo[-1] [ 1, 2, 3, 4, 5];
{
    "Status"   : "Error",
    "Message"  : "Index -1 is out of bounds for array of length 0"
}

numstore> insert foo[10] [ 1, 2, 3, 4, 5];
{
    "Status"   : "Error",
    "Message"  : "Index 10 is out of bounds for array of length 0"
}

numstore> insert foo [1, 2, 3, 4, 5, 6, 7, 8, 9];
{ "Status" : "Ok" }

numstore> insert foo[10] [ 1, 2, 3, 4, 5];
{
    "Status"   : "Error",
    "Message"  : "Index 10 is out of bounds for array of length 9"
}

numstore> insert foo[-10] [ 1, 2, 3, 4, 5];
{
    "Status"   : "Error",
    "Message"  : "Index -10 is out of bounds for array of length 9"
}

numstore> insert foo[-9] [ 1, 2, 3, 4, 5];
{ "Status"   : "Ok" }
```

### WRITE

While `INSERT` increases the array length, `WRITE` simply overwrites
data in place. To write data, you specify the _range_ you wish to modify.

```
numstore> write foo[0:10:12] [ 1, 2, 3, 4, 5 ];
{ "Status" : "Ok" }
```

The general form of a write command looks like this:

```
write <Variable Ref> [ data ];
```

A `Variable Ref` is discussed more in TODO varref, but 99% of cases,
a write command looks like this:

```
write <Variable Name>[ "[" [ start ] ":" [ stop ] [ ":" [ step ] ] "]" ] [ data ];
```

Every part of the range is optional:

* _start_ (`0` in the example above) is the first index written to. If it isn't
  included, it defaults to `0`. Negative numbers count back from the end of the
  array, -1 being the last index.
* _stop_ (`10` in the example above) is the index to stop at, **exclusive**. If it
  isn't included, it defaults to the length of the array. Negative numbers count
  back from the end.
* _step_ (`12` in the example above) is the distance between written indexes. If it
  isn't included, it defaults to `1`. A step of `0` is an error.
* _data_ is an optional explicit data source. See TODO DATA SECTION for more information
  on how numstore manages data input and output streams.

Assuming `foo` is a 5 element array, the following commands do the same thing:

```
numstore> write foo[0:5:1] [ 1, 2, 3, 4, 5 ];
{ "Status" : "Ok" }

numstore> write foo[0:] [ 1, 2, 3, 4, 5 ];
{ "Status" : "Ok" }

numstore> write foo[:] [ 1, 2, 3, 4, 5 ];
{ "Status" : "Ok" }

numstore> write foo [ 1, 2, 3, 4, 5 ];
{ "Status" : "Ok" }
```

For example, if we write into a 6 element array over the range `2:5`, the data
lands on top of what was already there. Nothing moves, and the array is the same
length afterwards:

```
write foo[2:5] [7, 8, 9]
Before:
  [1, 2, 3, 4, 5, 6]
         ^  ^  ^
        [7, 8, 9]
After:
  [1, 2, 7, 8, 9, 6]
```

Compare that to `insert foo[2] [7, 8, 9]`, which pushes the old data out of the
way and leaves you with a 9 element array. If you want to grow an array, you want
`INSERT`.

A _step_ spreads the write out over the array instead of laying it down
contiguously:

```
write foo[0:6:2] [7, 8, 9]
Before:
  [1, 2, 3, 4, 5, 6]
   ^     ^     ^
   7     8     9
After:
  [7, 2, 8, 4, 9, 6]
```

A negative _step_ walks the array backwards. Negative steps aren't supported yet but 
a planned feature.

```
numstore> write foo[0:6:-2] [7, 8, 9]
{ 
    "Status"  : "Error",
    "Message" : "Negative Strides aren't supported yet"
}
```

The range and the data don't have to agree on length. `WRITE` pairs them up one
element at a time and stops as soon as either one runs out. If the data runs out
first, the rest of the range is left exactly as it was:

```
write foo[:] [7, 8]
Before:
  [1, 2, 3, 4, 5, 6]
After:
  [7, 8, 3, 4, 5, 6]
```

And if the range runs out first, the remaining data is simply not consumed:

```
write foo[0:2] [7, 8, 9, 10, 11]
Before:
  [1, 2, 3, 4, 5, 6]
After:
  [7, 8, 3, 4, 5, 6]
```

An empty range is legal and writes nothing. In particular, writing to the range
just past the end of an array is **not** an `append`, it's a no-op:

```
numstore> write foo[6:] [7, 8, 9];
{ "Status" : "Ok" }

Before:
  [1, 2, 3, 4, 5, 6]
After:
  [1, 2, 3, 4, 5, 6]
```

Like `INSERT`, `WRITE` is strongly indexed. _start_ must be a valid index, and
_stop_ may be at most the length of the array (one past the last index):

```
numstore> create foo;
{ "Status" : "Ok" }

numstore> write foo[0] [ 1 ];
{
    "Status"   : "Error",
    "Message"  : "Index 0 is out of bounds for array of length 0"
}

numstore> insert foo [1, 2, 3, 4, 5, 6];
{ "Status" : "Ok" }

numstore> write foo[6] [ 1 ];
{
    "Status"   : "Error",
    "Message"  : "Index 6 is out of bounds for array of length 6"
}

numstore> write foo[0:7] [ 1, 2, 3 ];
{
    "Status"   : "Error",
    "Message"  : "Index 7 is out of bounds for array of length 6"
}

numstore> write foo[-7:] [ 1, 2, 3 ];
{
    "Status"   : "Error",
    "Message"  : "Index -7 is out of bounds for array of length 6"
}

numstore> write foo[0:6:0] [ 1, 2, 3 ];
{
    "Status"   : "Error",
    "Message"  : "Step cannot be 0"
}

numstore> write foo[-6:] [ 1, 2, 3 ];
{ "Status" : "Ok" }
```

### READ

To `READ` data, you specify the _range_ you wish to read.

```
numstore> read foo[0:10:12];
{ 
    "Status" : "Ok",
    "Data"   : [ 0, 1, 2 ]
}
```

The general form of a read command looks like this:

```
read <Variable Ref>;
```

A `Variable Ref` is discussed more in TODO varref, but 99% of cases,
a read command looks like this:

```
read <Variable Name>[ "[" [ start ] ":" [ stop ] [ ":" [ step ] ] "]" ];
```

`READ` has the same range semantics as `WRITE`. See TODO Write link

Where `WRITE` puts data into the range, `READ` takes it out. `Data` in the
response is the default output stream. See TODO DATA SECTION for more information
on how numstore manages data input and output streams.

Assuming `foo` is a 5 element array, the following commands do the same thing:

```
numstore> read foo[0:5:1];
{ "Status" : "Ok", "Data" : [ 1, 2, 3, 4, 5 ] }

numstore> read foo[0:];
{ "Status" : "Ok", "Data" : [ 1, 2, 3, 4, 5 ] }

numstore> read foo[:];
{ "Status" : "Ok", "Data" : [ 1, 2, 3, 4, 5 ] }

numstore> read foo;
{ "Status" : "Ok", "Data" : [ 1, 2, 3, 4, 5 ] }
```

For example, reading the range `2:5` out of a 6 element array picks up the three
elements the range covers. Reading never modifies the array:

```
read foo[2:5]

Before:
  [1, 2, 3, 4, 5, 6]
         ^  ^  ^

Data:
  [3, 4, 5]

After:
  [1, 2, 3, 4, 5, 6]
```

A _step_ spreads the read out over the array, the same way it spreads out a
write:

```
read foo[0:6:2]

Before:
  [1, 2, 3, 4, 5, 6]
   ^     ^     ^

Data:
  [1, 3, 5]
```

Negative steps aren't supported yet, so you can't read an array backwards:

```
numstore> read foo[0:6:-2];
{ 
    "Status"  : "Error",
    "Message" : "Negative Strides aren't supported yet"
}
```

An empty range is legal and reads nothing. Reading the range just past the end of
an array gives you back an empty array, not an error:

```
numstore> read foo[6:];
{ "Status" : "Ok", "Data" : [] }
```

Like `WRITE`, `READ` is strongly ranged. _start_ must be a valid index, and _stop_
may be at most the length of the array (one past the last index):

```
numstore> create foo;
{ "Status" : "Ok" }

numstore> read foo[0];
{
    "Status"   : "Error",
    "Message"  : "Index 0 is out of bounds for array of length 0"
}

numstore> insert foo [1, 2, 3, 4, 5, 6];
{ "Status" : "Ok" }

numstore> read foo[6];
{
    "Status"   : "Error",
    "Message"  : "Index 6 is out of bounds for array of length 6"
}

numstore> read foo[0:7];
{
    "Status"   : "Error",
    "Message"  : "Index 7 is out of bounds for array of length 6"
}

numstore> read foo[-7:];
{
    "Status"   : "Error",
    "Message"  : "Index -7 is out of bounds for array of length 6"
}

numstore> read foo[0:6:0];
{
    "Status"   : "Error",
    "Message"  : "Step cannot be 0"
}

numstore> read foo[-6:];
{ "Status" : "Ok", "Data" : [ 1, 2, 3, 4, 5, 6 ] }
```

### REMOVE 

To `REMOVE` data, you specify the _range_ you wish to remove. 

```
numstore> remove foo[0:10:12];
{
    "Status" : "Ok"
}
```

The general form of a remove command looks like this:

```
remove <Variable Name>[ "[" [ start ] ":" [ stop ] [ ":" [ step ] ] "]" ];
```

`REMOVE` has the same range semantics as `READ`. See TODO Write link

`REMOVE` is the other half of `INSERT`: it's the only operation besides `INSERT`
that changes the length of an array. Everything the range covers is taken out and
whatever is left closes the gap.

Because the whole range is optional, the following commands all remove
everything:

```
numstore> remove foo[0::1];
{ "Status" : "Ok" }

numstore> remove foo[0:];
{ "Status" : "Ok" }

numstore> remove foo[:];
{ "Status" : "Ok" }

numstore> remove foo;
{ "Status" : "Ok" }
```

This empties `foo` out, it doesn't get rid of `foo` itself. You're left with an
array of length 0:

```
remove foo

Before:
  [1, 2, 3, 4, 5, 6]
   ^  ^  ^  ^  ^  ^

After:
  []
```

For example, removing the range `2:5` from a 6 element array takes out three
elements and leaves a 3 element array behind:

```
remove foo[2:5]

Before:
  [1, 2, 3, 4, 5, 6]
         ^  ^  ^

After:
  [1, 2, 6]
```

A _step_ punches holes in the array instead of taking out one contiguous block.
The array still closes up afterwards:

```
remove foo[0:6:2]

Before:
  [1, 2, 3, 4, 5, 6]
   ^     ^     ^

After:
  [2, 4, 6]
```

Negative steps aren't supported yet:

```
numstore> remove foo[0:6:-2];
{ 
    "Status"  : "Error",
    "Message" : "Negative Strides aren't supported yet"
}
```

An empty range is legal and removes nothing:

```
numstore> remove foo[6:];
{ "Status" : "Ok" }

Before:
  [1, 2, 3, 4, 5, 6]
After:
  [1, 2, 3, 4, 5, 6]
```

Like `READ`, `REMOVE` is strongly ranged. _start_ must be a valid index, and
_stop_ may be at most the length of the array (one past the last index):

```
numstore> create foo;
{ "Status" : "Ok" }

numstore> remove foo[0];
{
    "Status"   : "Error",
    "Message"  : "Index 0 is out of bounds for array of length 0"
}

numstore> insert foo [1, 2, 3, 4, 5, 6];
{ "Status" : "Ok" }

numstore> remove foo[6];
{
    "Status"   : "Error",
    "Message"  : "Index 6 is out of bounds for array of length 6"
}

numstore> remove foo[0:7];
{
    "Status"   : "Error",
    "Message"  : "Index 7 is out of bounds for array of length 6"
}

numstore> remove foo[-7:];
{
    "Status"   : "Error",
    "Message"  : "Index -7 is out of bounds for array of length 6"
}

numstore> remove foo[0:6:0];
{
    "Status"   : "Error",
    "Message"  : "Step cannot be 0"
}

numstore> remove foo[-6:];
{ "Status" : "Ok" }
```

The Numstore Type System
------------------------


Variable References
-------------------


Variable Constructions
----------------------


