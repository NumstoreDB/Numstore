# What is Numstore?

Numstore is a database for arrays. It is an ACID database meant to store
contiguous arrays of numerical values. 

Features include:

1. Transaction Support 
2. Durability on power failure 
3. O(log N) inner array mutations (insert / remove)
4. A simple but extendable type system that mimics numpy's dtype

## Why Numstore?

Relational databases store tables: rows of named, typed columns, like a `users`
table with a name and a date of birth. They are a poor fit for data where one
value is a large block of numbers:

* a 3x256x256 RGB image
* a stream of thousands of floating point stock prices

That data usually ends up in flat binary files or formats like HDF5, which are
fast but give you no transactions: a crash halfway through a write leaves the
file half written. Numstore stores the same data with database guarantees.
