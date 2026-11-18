Numstore documentation
======================

Numstore is an ACID database for arrays. Here's a map of the docs.

Start here
----------

* [Getting started](getting-started.md) - install Numstore and run your first
  queries in Python or C.
* [Why Numstore](concepts/why-numstore.md) - what Numstore is for, and how it
  differs from a relational database.
* [Glossary](glossary.md) - terms these docs use.

Tutorials
---------

Tutorials teach Numstore step by step. You run every example as you go.

* [The query language](tutorials/query-language.md) - make variables, then
  insert, write, read and remove data.
* [Transactions](tutorials/transactions.md) - group queries, then commit or
  roll back.
* [Type sizes](tutorials/type-sizes.md) - work out the size of a type, then
  check your answers.
* [Crash recovery](tutorials/crash-recovery.md) - crash a process and see what
  Numstore keeps.

Guides
------

Guides are short instructions for one task each.

The C library:

* [Build and link the C library](guides/build-and-link.md)
* [Handle errors](guides/handle-errors.md)
* [Use transactions in C](guides/transactions-in-c.md)
* [Read data in C](guides/read-data-in-c.md)
* [Store structs and arrays from C](guides/structs-and-arrays-in-c.md)

Working with data:

* [Inspect variables](guides/inspect-variables.md)
* [Reuse a query with a plan](guides/reuse-queries.md)
* [Move data in and out of Python](guides/move-data-in-python.md)
* [Back up a database](guides/back-up-a-database.md)

Concepts
--------

Concept pages explain how Numstore works and why.

* [The type system](concepts/types.md)
* [Transactions and concurrency](concepts/transactions-and-concurrency.md)
* [Durability and crash recovery](concepts/durability.md)
* [The error model](concepts/errors.md)
