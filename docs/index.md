Numstore Documentation
======================

Numstore is an ACID database for arrays. This page is a summary of the 
Numstore docs.

Start here
----------

* [**Getting started**](GETTING_STARTED.md) - install Numstore and run your
  first queries in Python or C.
* [**Glossary**](GLOSSARY.md) - the words that these documents use.

Tutorials
---------

You learn best by doing.

* [The query language](tutorials/language_tour.md) - make variables, then
  insert, write, read and remove data.
* [Transactions](tutorials/transactions.md) - group queries, then commit or
  roll back.
* [Type sizes](tutorials/type_sizes.md) - calculate the size of a type. Then
  check your answers.
* [Crash recovery](tutorials/crash_recovery.md) - crash a process and see what
  Numstore keeps.

How-to guides
-------------

You want straight to the point information on how to use Numstore.

* [Build and link the C library](howto/build_and_link.md)
* [Handle errors](howto/handle_errors.md)
* [Use transactions in C](howto/use_transactions_in_c.md)
* [Read a large variable in chunks](howto/read_in_chunks.md)
* [Let Numstore allocate the buffer](howto/let_numstore_allocate.md)
* [Store structs and arrays from C](howto/store_structs_in_c.md)
* [Inspect variables](howto/inspect_variables.md)
* [Reuse a query with a plan](howto/reuse_queries.md)
* [Use smart files](howto/use_smart_files.md)
* [Move data in and out from Python](howto/move_data_in_python.md)
* [Back up a database](howto/back_up_a_database.md)

Concepts
--------

Numstore concepts and ideas

* [Why Numstore](concepts/why_numstore.md)
* [The type system](concepts/types.md)
* [Transactions and concurrency](concepts/transactions_and_concurrency.md)
* [Durability and crash recovery](concepts/durability.md)
* [The error model](concepts/errors.md)
