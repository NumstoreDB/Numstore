Dependency Chart
================

```
numstore-rs                                  pynumstore
     |                                            |
     v                                            |
ns_numstore                                       |
     |                                            |
     +------------------------+-------------------+
                              |
                              v
                           ns_nsdb
                              |
                              v
                         algorithms
                              |
                    +---------+--------+
                    |                  |
                    v                  v
                ns_nscore          ns_types
                    |                  |
         +----------+----------+       |
         |          |          |       |
         v          v          v       |
   ns_disk_pager ns_disk_wal ns_page <--+
         |          |          |
         +----------+----------+
                    |
                    v
                ns_common
                    |
                    v
               pthreads, m
```

Why is it organized like that?
==============================
Numstore used to be a single file library libnumstore.a however, I wanted to
push to start re writing and adding to numstore using rust. Therefore, numstore
broke apart  into smaller sub libraries that interface with each other to lower
to target of "re write component x" in rust.

It encourages numstore to adopt clear library boundaries to encourage ffi
bindings in rust.

Libraries
=========

numstore-rs
-----------
Rust bindings for numstore. These "wrap" numstore, they don't "contribute to"
numstore. This is your first point of entry for numstore if you're using rust.

pynumstore
----------
Python bindings for numstore. These "wrap" numstore, and specifically only the
nsdb layer - and avoid using ns_numstore so that we get access to error
handling.

algorithms
----------
All the database access patterns for numstore. Working with ropes, variables
hash maps etc.

common
------
Common C code

disk_pager
----------
A direct to file interface for the pager. I have plans to write a multi file
pager, with the same interface as disk_pager. This could be written in rust.

disk_wal
--------
A direct to file interface for the wal. Like disk_pager, I have plans to write
a multi file pager.

nscore
------
The core of the database recovery and transactional logic in numstore

nsdb
----
An internal database tool that exposes all the innards of numstore like errors
etc. Most public facing apis use nsdb and hide internals like errors

nslang
------
The compiler and type system for numstore. This could be re written in rust -
or at least the compiler.

numstore
--------
A public facing library that's self contained.

page
----
The library for all different page types
