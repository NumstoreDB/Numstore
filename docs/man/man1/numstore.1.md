---
title: NUMSTORE
section: 1
header: Numstore Manual
footer: numstore 1.0
date: September 2026
---

# NAME

numstore - ACID database and REPL for contiguous numerical arrays

# SYNOPSIS

**numstore** [*DATABASE*]

**numstore>** *COMMAND* [*ARGUMENTS*]**;**

# DESCRIPTION

**numstore** is an ACID database built to store contiguous arrays of
numerical values, rather than the tabular, record-oriented data that
relational databases target. A **numstore** database is a map from
variable names to arrays. Each variable holds exactly one array of a
fixed element type; that type is chosen when the variable is created
and does not change.

Unlike relational systems, **numstore** does not use SQL. Its query
language has seven operations, split into two groups:

*Variable operations* - act on a variable itself:

**CREATE**, **GET**, **DELETE**

*Array operations* - act on the contents of a variable's array:

**INSERT**, **WRITE**, **READ**, **REMOVE**

Interaction happens through the **numstore** REPL. Each command is
terminated with a semicolon and returns a JSON object describing the
result. On success the object contains `"Status": "Ok"`; on failure it
contains `"Status": "Error"` and a `"Message"` field.

# COMMANDS

## CREATE

**create** *name* *type*\;

Creates a new, empty variable named *name* with element type *type*
(see **TYPE SYSTEM** below). Fails if a variable with that name
already exists.

    numstore> create foo u32;
    { "Status" : "Ok" }

## GET

**get** [**if exists**] *name*\;

Returns metadata about the variable *name*: its **Name**, **DType**,
**DSize** (size in bytes of one element), **Nelems** (element count),
**Bytes** (total size), and **Root** (the page number where the
variable's data begins, or *null* if the variable is empty).

With **if exists**, returns success with no error if the variable is
absent, rather than failing.

    numstore> get foo;
    {
        "Status" : "Ok",
        "Name"   : "foo",
        "DType"  : "u32",
        "DSize"  : 4,
        "Nelems" : 1296,
        "Bytes"  : 5184,
        "Root"   : 12398312
    }

## DELETE

**delete** *name*\;

Removes the variable *name* and all of its data. Fails if no such
variable exists.

## INSERT

**insert** *name*\[*offset*\] [*data*]\;

Inserts *data* into the array at *offset*, shifting existing elements
at or after *offset* to make room. **INSERT** is the only operation
(besides **REMOVE**) that changes an array's length.

- *offset* - where to insert. Omitted, it appends to the end.
  Negative values count back from the end, with *-1* the last index.
- *data* - the values to insert.

*offset* is strongly indexed: it must satisfy
*-len \<= offset \<= len*, or the command fails with an
out-of-bounds error.

    numstore> insert foo[0] [ 1, 2, 3, 4, 5 ];
    { "Status" : "Ok" }

    numstore> insert foo [ 6, 7, 8 ];
    { "Status" : "Ok" }

## WRITE

**write** *name*\[[*start*]:[*stop*][:[*step*]]\] [*data*]\;

Overwrites elements in the given range with *data*, in place.
**WRITE** never changes the array's length.

- *start* - first index written; defaults to *0*.
- *stop* - index to stop before, exclusive; defaults to the array
  length.
- *step* - distance between written indexes; defaults to *1*. A step
  of *0* is an error. Negative steps are not yet supported.
- *data* - the replacement values.

The range and *data* are paired element-by-element and stop as soon
as either runs out; leftover range positions are left unchanged, and
leftover data is discarded. An empty range, including the range
starting exactly at the array's length, is legal and writes nothing
- it is not equivalent to **INSERT**.

*start* must be a valid index and *stop* may be at most the array
length (one past the last valid index), or the command fails.

    numstore> write foo[0:5:1] [ 1, 2, 3, 4, 5 ];
    { "Status" : "Ok" }

    numstore> write foo[0:6:2] [ 7, 8, 9 ];
    { "Status" : "Ok" }

## READ

**read** *name*\[[*start*]:[*stop*][:[*step*]]\]\;

Returns the elements in the given range as a `"Data"` array, without
modifying the variable. Range semantics are identical to **WRITE**;
see above. Reading a range beyond or exactly at the array's end
returns an empty **Data** array, not an error.

    numstore> read foo[0:5:1];
    { "Status" : "Ok", "Data" : [ 1, 2, 3, 4, 5 ] }

## REMOVE

**remove** *name*\[[*start*]:[*stop*][:[*step*]]\]\;

Removes the elements in the given range and closes the resulting gap,
shortening the array. Range semantics are identical to **READ**; see
above. **REMOVE** is the other length-changing operation besides
**INSERT**. Since every part of the range is optional, **remove**
*name*\; with no range removes all elements, leaving an empty array
(the variable itself still exists).

    numstore> remove foo[2:5];
    { "Status" : "Ok" }

    numstore> remove foo;
    { "Status" : "Ok" }

# TYPE SYSTEM

Every **numstore** type is built from four kinds: **Primitive**,
**Struct**, **Union**, and **Strict Array**. Structs, unions, and
arrays combine other types; a **Primitive** is always the base case.
Sizes compose recursively using four rules:

    Primitive     bits / 8
    Struct        sum of member sizes
    Union         max of member sizes
    Strict Array  product of dimensions x element size

## Primitives

Primitives are named by signedness, float/integer, real/complex, and
bit width. Byte size is bits divided by 8. A complex type packs a
real part and an imaginary part back to back, so its bit width is
double that of the real type it is built from.

    Type     Bits  Bytes  Notes
    i8        8     1     signed integer
    i16      16     2     signed integer
    i32      32     4     signed integer
    i64      64     8     signed integer
    u8        8     1     unsigned integer
    u16      16     2     unsigned integer
    u32      32     4     unsigned integer
    u64      64     8     unsigned integer
    f16      16     2     float
    f32      32     4     float
    f64      64     8     float
    f128    128    16     float
    ci16     16     2     complex i8  (2 x i8)
    ci32     32     4     complex i16 (2 x i16)
    ci64     64     8     complex i32 (2 x i32)
    ci128   128    16     complex i64 (2 x i64)
    cu16     16     2     complex u8  (2 x u8)
    cu32     32     4     complex u16 (2 x u16)
    cu64     64     8     complex u32 (2 x u32)
    cu128   128    16     complex u64 (2 x u64)
    cf32     32     4     complex f16 (2 x f16)
    cf64     64     8     complex f32 (2 x f32)
    cf128   128    16     complex f64 (2 x f64)
    cf256   256    32     complex f128 (2 x f128)

    numstore> create foo u32;
    { "Status" : "Ok" }

## Structs

**struct { *member* *type*, ... }**

A product type; members are laid out back to back with no padding.
Size is the sum of member sizes.

    numstore> create foo struct { a u32, b cf256 };
    { "Status" : "Ok" }

`a` occupies bytes *0-3*; `b` starts immediately after at byte *4*.
Total **DSize** is *4 + 32 = 36* bytes.

## Unions

**union { *member* *type*, ... }**

A sum type; every member starts at byte offset *0* and overlays the
same storage, with no padding. Size is the size of the largest
member.

    numstore> create foo union { a u32, b cf256 };
    { "Status" : "Ok" }

Both `a` and `b` begin at byte *0*. **DSize** is
*max(4, 32) = 32* bytes. Reading `a` interprets the first 4 bytes as
a `u32`; reading `b` interprets all 32 bytes as a `cf256`.

## Strict Arrays

**[*dim*]...[*dim*]** *type*

A fixed-shape array. Dimensions are read left to right, outermost
first: `[256][256] f32` is a 256-element array of 256-element arrays
of `f32`. Size is the product of all dimensions times the element
size.

    numstore> create foo [256][256] f32;
    { "Status" : "Ok" }

**DSize** is *256 x 256 x 4 = 262144* bytes.

### Array ordering (C order)

Strict arrays are stored in **C order** (row-major): the last
dimension is contiguous in memory, and the last index varies fastest.
For a *[R][C]* array of element size *S*, element *[i][j]* sits at
byte offset:

    offset = (i * C + j) * S

This generalizes to any number of dimensions by multiplying each
index by the product of the dimensions to its right, summing, and
multiplying by the element size.

## Combining types

Struct members, union members, and array elements may themselves be
structs, unions, or arrays, nested arbitrarily deep; the four sizing
rules apply recursively at every level.

    numstore> create points [1000] struct { x f32, y f32, z f32 };
    { "Status" : "Ok" }

    numstore> create frame struct { timestamp u64, pixels [256][256] u8 };
    { "Status" : "Ok" }

    numstore> create video [12][256][256] struct { r u8, g u8, b u8 };
    { "Status" : "Ok" }

# EXAMPLES

Create a variable, insert data, read it back, then trim it down:

    numstore> create foo u32;
    { "Status" : "Ok" }

    numstore> insert foo [ 1, 2, 3, 4, 5, 6 ];
    { "Status" : "Ok" }

    numstore> read foo[:];
    { "Status" : "Ok", "Data" : [ 1, 2, 3, 4, 5, 6 ] }

    numstore> remove foo[2:5];
    { "Status" : "Ok" }

    numstore> read foo[:];
    { "Status" : "Ok", "Data" : [ 1, 2, 6 ] }

    numstore> get foo;
    {
        "Status" : "Ok",
        "Name"   : "foo",
        "DType"  : "u32",
        "DSize"  : 4,
        "Nelems" : 3,
        "Bytes"  : 12,
        "Root"   : 12398312
    }

# ERRORS

Errors are returned in-band as JSON, not as REPL exit codes. A
failing command returns `"Status": "Error"` and a `"Message"`
describing the problem. Common failure cases:

- Out-of-bounds *offset* or *start*/*stop* in **INSERT**, **WRITE**,
  **READ**, or **REMOVE**.
- A *step* of *0* in **WRITE**, **READ**, or **REMOVE**.
- A negative *step* in **WRITE**, **READ**, or **REMOVE** (not yet
  supported).
- **CREATE** on a name that already exists.
- **GET** or **DELETE** on a name that does not exist (unless
  **if exists** is given to **GET**).

# SEE ALSO

**hdf5**(3), **sqlite3**(1)

# AUTHOR

Written by Theo Lincke.

# COPYRIGHT

Copyright 2026 Theo Lincke. Licensed under the Apache License,
Version 2.0. See *http://www.apache.org/licenses/LICENSE-2.0* for
details.
