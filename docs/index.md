Numstore Documentation
======================

Numstore is an ACID database for arrays. This page shows where each document
is.

Start here
----------

* [**Getting started**](GETTING_STARTED.md) - install Numstore and run your
  first queries in Python or C.
* [**Glossary**](GLOSSARY.md) - the words that these documents use.

Tutorials
---------

Tutorials teach. Do the steps in order.

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

A how-to guide does one task.

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

A concept page tells why Numstore works as it does.

* [Why Numstore](concepts/why_numstore.md)
* [The type system](concepts/types.md)
* [Transactions and concurrency](concepts/transactions_and_concurrency.md)
* [Durability and crash recovery](concepts/durability.md)
* [The error model](concepts/errors.md)

Reference
---------

* [**C API**](reference/c_api.md) - every function, type and constant in
  `numstore.h`.
* [**Python API**](reference/python_api.md) - every class and method in
  `pynumstore`.

Samples
-------

Programs that you can run are in [`samples/`](../samples):
[`sample1_basic.py`](../samples/sample1_basic.py),
[`sample2_datatypes.py`](../samples/sample2_datatypes.py),
[`sample3_transactions.py`](../samples/sample3_transactions.py),
[`sample4_crash.py`](../samples/sample4_crash.py) and
[`sample5_errors.py`](../samples/sample5_errors.py).
