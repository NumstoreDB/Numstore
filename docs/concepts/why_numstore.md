Why Numstore
============

What is Numstore?
-----------------

Numstore is a database for arrays. It is an ACID database. It stores
contiguous arrays of numbers.

Its features are:

1. Transactions
2. Durability on power failure
3. O(log N) changes inside an array (insert and remove)
4. A simple type system that you can extend. It is like the numpy dtype.

How is it different from other databases?
-----------------------------------------

SQL is very good. It does many tasks. For record data, SQL solves the storage
problem very well. Numstore does not compete with SQL. Numstore stores a
different shape of data: _arrays_.

A relational database puts data into _tables_. A table is one _type_. A table
has _columns_. Each column has a name. To get data, you give the tables and
the columns that you want. This is tabular data.

Do you put a 256x256 pixel image in a relational database? Here are some ways
to do it:

1. Put each pixel in its own column, up to pixel 65536. This fails because of
   column limits. Postgres permits 1600 columns. For small arrays, this idea
   is attractive. You can put an array of 5 floats in 5 columns. But usually,
   the "names" of the columns of an array are only its indexes.

2. Store a reference to the data in the file system. This is probably the most
   usual way to store data with many dimensions in a database. You store the
   location of the file on disk (for example
   `file:///foo/bar/biz/image.png`), or on the internet (for example
   `https://amazonaws.com/foo/bar/biz/image.png`). This works, but now there
   are two sources of truth. If you move the image and do not update the
   database, the data is wrong. You lose all of the ACID properties of the
   database, because the file system is not reliable.

3. Store the image data in a column type, for example `BYTEA`. This solves some
   of the problems of (2). But large files in a database column are often not
   a good idea. They make the database larger and use more memory. Relational
   databases were not made for this type of data.

Relational databases _do not support packed data with many dimensions_. This
is a basic limit. Some file formats store this data:

1. HDF5
2. Parquet
3. Zarr-Python

But each of these formats says that it is not a database, because:

1. A database must support readers and writers at the same time, with logical
   isolation. Two threads do not know about the work of the other thread on
   the same data. But the database stays consistent.

2. A database must support transactions. HDF5 is atomic, but you cannot do
   many operations on an HDF5 file atomically.

Numstore is the database for this type of data.
