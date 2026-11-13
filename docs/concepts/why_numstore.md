Why Numstore
============

What is Numstore?
-----------------

Numstore is a database for arrays. It is an ACID database meant to store
contiguous arrays of numerical values.

Features include:

1. Transaction Support
2. Durability on power failure
3. O(log N) inner array mutations (insert / remove)
4. A simple but extendable type system that mimics numpy's dtype

How is it different than other databases?
-----------------------------------------

SQL is great. It's versatile and in record-oriented computing, SQL solves the
data storage problem very well. Numstore doesn't compete with SQL. Numstore is
meant to store a different shape of data: _arrays_.

In traditional relational databases, data is organized into _Tables_ (a
singular _type_) which are further organized into _columns_. Columns are named
quantities. Each column has a name, and to access certain data you specify what
table(s) you want and what column(s) you want. This is tabular data.

Would you ever store a 256x256 pixel image in a relational database? Let's
think of ways to do this:

1. Store each pixel in its own column - up to pixel 65536. This breaks down
   easily with column limits. Postgres limits columns to 1600. For small
   arrays this is a tempting idea. A 5 element long array of floats could just
   be stored as 5 columns but in general, arrays are things where the "names"
   of columns are implicitly the index where they reside.

2. Storing a reference to the file system data. This is probably the most
   common approach to storing high-dimensionality data in databases. You store
   the location of the file on disk (e.g. 'file:///foo/bar/biz/image.png') or
   where it is on the internet
   (e.g. 'https://amazonaws.com/foo/bar/biz/image.png'). This is nice, but now
   you have two sources of truth. What happens if you move the image and don't
   update the database? Fundamentally, you're removing all the ACID properties
   of the database in favor of just storing the image in an unreliable file
   system.

3. Store the image data as some internal column type. We could store the image
   pixels as a BYTEA or something similar. This solves some of the problems of
   (2), but it doesn't always make sense to store large files in a database
   column. This often bloats database size and degrades memory consumption
   because fundamentally, relational databases weren't built for that type of
   data.

A fundamental limitation of relational databases is that _they don't support
packed high-dimensional data natively_. There are file formats out there:

1. HDF5
2. Parquet
3. Zarr-Python

But each one makes it very clear that it isn't a database because:

1. Databases should support concurrent writers / readers and logical isolation
   meaning two threads don't know about each other's work on the same data but
   keep the database in a consistent state.

2. Databases should support transactions - sure HDF5 is atomic, but you can't
   do multiple things to an HDF5 file atomically.

Numstore is the database for this type of data.
