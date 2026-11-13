C API Reference
===============

Numstore is a C11 library. It has one public header,
`libs/numstore/include/numstore.h`, and a set of static archives that you link
together. It needs only libc, pthreads and libm.

If Numstore is new to you, read [Getting started](../GETTING_STARTED.md) first.
Then read the [query language tutorial](../tutorials/language_tour.md). The
tutorial uses Python, but the queries are the same in C. Only the host language
changes. For the Python bindings, see [the Python API](python_api.md).

Each program in these documents is complete. It compiles as shown. Each output
block is the real output of the program.

* [Function reference](#function-reference)
* [Types and constants](#types-and-constants)

Guides for C
------------

These pages hold the C examples:

| Task                                    | Page                                                                    |
| --------------------------------------- | ----------------------------------------------------------------------- |
| Build and link                          | [Build and link](../howto/build_and_link.md)                            |
| A first program                         | [Getting started](../GETTING_STARTED.md#a-first-program)                |
| Find and report errors                  | [Handle errors](../howto/handle_errors.md)                              |
| Where errors go, and who owns them      | [The error model](../concepts/errors.md)                                |
| Group queries                           | [Use transactions in C](../howto/use_transactions_in_c.md)              |
| Durability and crash recovery           | [Durability](../concepts/durability.md)                                 |
| See a crash recovery                    | [Crash recovery tutorial](../tutorials/crash_recovery.md)               |
| Read into a buffer that you own         | [Read in chunks](../howto/read_in_chunks.md)                            |
| Read into a buffer that Numstore makes  | [Let Numstore allocate](../howto/let_numstore_allocate.md)              |
| Structs and arrays                      | [Store structs in C](../howto/store_structs_in_c.md)                    |
| Variable metadata                       | [Inspect variables](../howto/inspect_variables.md)                      |
| Prepared plans                          | [Reuse queries](../howto/reuse_queries.md)                              |
| Smart files                             | [Use smart files](../howto/use_smart_files.md)                          |
| Concurrency and threading               | [Transactions and concurrency](../concepts/transactions_and_concurrency.md) |

Function reference
------------------

### Lifecycle

| Function                                                                            | Returns                   | Notes                                                                                 |
| ----------------------------------------------------------------------------------- | ------------------------- | ------------------------------------------------------------------------------------- |
| `nsdb_t *ns_open (const char *path)`                                                | handle, `NULL` on failure | opens or creates a database                                                           |
| `nsdb_t *ns_open_with_error (const char *path, char erstr[], unsigned elen)`        | handle, `NULL` on failure | same, and copies the failure message into `erstr`; zero the buffer first              |
| `nsdb_t *ns_smfile_open (const char *path)`                                         | handle, `NULL` on failure | opens or creates a smart file                                                         |
| `nsdb_t *ns_smfile_open_with_error (const char *path, char erstr[], unsigned elen)` | handle, `NULL` on failure | same, with a message                                                                  |
| `int ns_close (nsdb_t *ns)`                                                         | `0` on success            | flushes and closes; blocks while a transaction is open                                |
| `int ns_crash (nsdb_t *ns)`                                                         | `0` on success            | drops the handle without flushing; open transactions are rolled back on the next open |
| `int ns_cleanup (const char *path)`                                                 | `0` on success            | deletes the database or smart file at `path`                                          |

### Queries

| Function                                                                                            | Returns                            | Notes                                                                 |
| --------------------------------------------------------------------------------------------------- | ---------------------------------- | --------------------------------------------------------------------- |
| `int ns_execute (nsdb_t *db, nsdb_txn_t *tx, const char *fmt, ...)`                                 | `0` on success                     | `CREATE` `DELETE` `REMOVE`                                            |
| `sb_size ns_read (nsdb_t *db, nsdb_txn_t *tx, void *dest, b_size dlen, const char *fmt, ...)`       | elements read, `< 0` on failure    | `READ` `REMOVE`; `dlen` is bytes                                      |
| `sb_size ns_write (nsdb_t *db, nsdb_txn_t *tx, const void *src, b_size slen, const char *fmt, ...)` | elements written, `< 0` on failure | `INSERT` `WRITE`; `slen` is bytes                                     |
| `void *ns_malloc (nsdb_t *db, nsdb_txn_t *tx, b_size *dlen, const char *fmt, ...)`                  | buffer, `NULL` on failure          | `READ` `REMOVE`; writes the byte count to `dlen`; release with `free` |
| `nsdb_var_t *ns_get_var (nsdb_t *db, nsdb_txn_t *tx, const char *query, ...)`                       | handle, `NULL` on failure          | any query naming a variable; the query never runs                     |

### Transactions

| Function                                        | Returns                   | Notes                                      |
| ----------------------------------------------- | ------------------------- | ------------------------------------------ |
| `nsdb_txn_t *ns_begin (nsdb_t *ns)`             | handle, `NULL` on failure | blocks while another transaction is open   |
| `int ns_commit (nsdb_t *ns, nsdb_txn_t *txn)`   | `0` on success            | durable when it returns; spends the handle |
| `int ns_rollback (nsdb_t *ns, nsdb_txn_t *txn)` | `0` on success            | undoes everything; spends the handle       |

### Variables

| Function                                                                                     | Returns        | Notes                                                                                       |
| -------------------------------------------------------------------------------------------- | -------------- | ------------------------------------------------------------------------------------------- |
| `b_size ns_var_len (nsdb_var_t *var)`                                                        | elements       | length when the handle was taken                                                            |
| `void ns_var_free (nsdb_var_t *var)`                                                         |                | frees a handle from `ns_get_var`; may run after `ns_close`                                  |
| `int ns_visit_nsdb_vars (nsdb_t *db, nsdb_var_consumer consumer, nsdb_txn_t *tx, void *ctx)` | `0` on success | calls `consumer (var, ctx)` per variable; the callback must not free `var`; `tx` is ignored |

### Plans

| Function                                                                                  | Returns                            | Notes                            |
| ----------------------------------------------------------------------------------------- | ---------------------------------- | -------------------------------- |
| `nsdb_plan_t *ns_plan_fcreate (nsdb_t *db, const char *fmt, ...)`                         | plan, `NULL` on failure            | compiles a query                 |
| `void ns_plan_free (nsdb_plan_t *plan)`                                                   |                                    | free before closing the database |
| `int ns_plan_execute (nsdb_plan_t *plan, nsdb_txn_t *tx)`                                 | `0` on success                     |                                  |
| `sb_size ns_plan_read (nsdb_plan_t *plan, nsdb_txn_t *tx, void *dest, b_size dlen)`       | elements read, `< 0` on failure    |                                  |
| `sb_size ns_plan_write (nsdb_plan_t *plan, nsdb_txn_t *tx, const void *src, b_size slen)` | elements written, `< 0` on failure |                                  |
| `void *ns_plan_malloc (nsdb_plan_t *plan, nsdb_txn_t *tx, b_size *dlen)`                  | buffer, `NULL` on failure          | release with `free`              |
| `nsdb_var_t *ns_plan_get_var (nsdb_plan_t *plan, nsdb_txn_t *tx)`                         | handle, `NULL` on failure          | free with `ns_var_free`          |

### Errors

| Function                                                               | Returns                 | Notes                                                  |
| ---------------------------------------------------------------------- | ----------------------- | ------------------------------------------------------ |
| `const char *ns_strerror (nsdb_t *ns, bool copy)`                      | message, `NULL` if none | takes the last failure on this handle, from any thread |
| `const char *ns_plan_strerror (nsdb_plan_t *plan, bool copy)`          | message, `NULL` if none | the same as `ns_strerror` on the plan's handle         |
| `const char *ns_txn_strerror (nsdb_t *ns, nsdb_txn_t *txn, bool copy)` | message, `NULL` if none | takes the last failure inside this transaction         |

Each call clears the error that it returns. If `copy` is true, the message is a
new string. Release it with `free`. If `copy` is false, the pointer is correct
only until the next failure. It is not safe on a handle that other threads use.
See [the error model](../concepts/errors.md).

### Smart files

| Function                                                                                                                           | Returns                            | Notes                                                          |
| ---------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------- | -------------------------------------------------------------- |
| `sb_size ns_smfile_size (nsdb_t *smf, nsdb_txn_t *tx)`                                                                             | bytes                              | size under `tx`'s view                                         |
| `sb_size ns_smfile_insert (nsdb_t *smf, nsdb_txn_t *tx, const void *src, sb_size bofst, b_size slen)`                              | bytes inserted, `< 0` on failure   | pushes `slen` bytes in at byte offset `bofst`                  |
| `sb_size ns_smfile_write (nsdb_t *smf, nsdb_txn_t *tx, const void *src, t_size size, sb_size bofst, sb_size stride, b_size nelem)` | elements written, `< 0` on failure | overwrites `nelem` elements of `size` bytes, stepping `stride` |
| `sb_size ns_smfile_read (nsdb_t *smf, nsdb_txn_t *tx, void *dest, t_size size, sb_size bofst, sb_size stride, b_size nelem)`       | elements read, `< 0` on failure    | same addressing, reading                                       |
| `sb_size ns_smfile_remove (nsdb_t *smf, nsdb_txn_t *tx, void *dest, t_size size, sb_size bofst, sb_size stride, b_size nelem)`     | elements removed, `< 0` on failure | removes and optionally hands the bytes back in `dest`          |

Types and constants
-------------------

| Name                | Underlying                        | What it is                                                                                         |
| ------------------- | --------------------------------- | -------------------------------------------------------------------------------------------------- |
| `nsdb_t`            | opaque                            | a database or smart file handle                                                                    |
| `nsdb_txn_t`        | opaque                            | a transaction                                                                                      |
| `nsdb_var_t`        | opaque                            | a variable metadata handle                                                                         |
| `nsdb_plan_t`       | opaque                            | a compiled query                                                                                   |
| `t_size`            | `uint32_t`                        | the size of one type, in bytes                                                                     |
| `b_size`            | `uint64_t`                        | a byte count or byte offset                                                                        |
| `sb_size`           | `int64_t`                         | a signed `b_size`; negative means failure                                                          |
| `nsdb_var_consumer` | `void (*) (nsdb_var_t *, void *)` | the `ns_visit_nsdb_vars` callback                                                                  |
| `MAX_ERRSTR_LEN`    | `256`                             | big enough for any Numstore error message                                                          |
| `NS_END`            | `INT64_MAX`                       | as a smart file `nelem`, "to the end of the file" - see the warning in [Use smart files](../howto/use_smart_files.md) |

Print `b_size` with `PRIu64`. Print `sb_size` with `PRId64`. Both come from
`<inttypes.h>`. `numstore.h` already includes it.
