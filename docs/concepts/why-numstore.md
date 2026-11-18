Why Numstore
============

What Numstore is
----------------

Numstore is an ACID database for arrays. It stores contiguous arrays of
numbers.

It has:

* Transactions
* Durability on power failure
* O(log N) inserts and removes in the middle of an array
* A small, extensible type system that mirrors NumPy's dtypes

How Numstore differs from other databases
-----------------------------------------

SQL works well for record-oriented data, and Numstore doesn't compete with it.
Numstore stores a different shape of data: _arrays_.

A relational database organizes data into _tables_, and each table into
_columns_. A column is a named quantity. To get data out, you say which tables
and which columns you want. This is tabular data.

Now try to store a 256x256 pixel image in a relational database. You have three
options:

1. Store each pixel in its own column, up to pixel 65536. This breaks down fast
   because of column limits: Postgres allows 1600 columns. For small arrays it's
   tempting. You could store a five element array of floats as five columns.
   But in general, the "names" of an array's columns are its indexes.

2. Store a reference to the file. This is probably the most common way to keep
   high-dimensional data next to a database. You store the location of the file
   on disk (for example, `file:///data/images/cat.png`) or on the internet (for
   example, `https://example.com/images/cat.png`). This works, but now you have
   two sources of truth. If you move the image and don't update the database,
   the reference breaks. The image also lives in a file system that can't give
   you the database's ACID guarantees.

3. Store the image data in a column, as a `BYTEA` or something similar. This
   solves some of the problems of option 2, but large values don't always
   belong in a database column. They often bloat the database and use more
   memory, because relational databases weren't built for that kind of data.

Relational databases don't support packed, high-dimensional data natively.
File formats exist for it:

* HDF5
* Parquet
* Zarr-Python

Each one is clear that it isn't a database. A database must:

1. Support concurrent readers and writers, with isolation: two threads don't see
   each other's work on the same data, and the database stays consistent.

2. Support transactions. HDF5 is atomic, but you can't do several things to an
   HDF5 file atomically.

Numstore is a database for this kind of data.
