# Data Model: Variables and Elements

A database holds **variables**. Each variable has:

* a **name**, like `prices`
* a **type**, the byte layout of one element, like `f64` 
* **data**, an array of elements of that type

You work with variables through short queries:

| Query                    | What it does                                     |
| ------------------------ | ------------------------------------------------ |
| `create prices f64`      | create a variable                                |
| `delete prices`          | delete a variable                                |
| `insert prices 0 3`      | insert 3 elements at position 0                  |
| `write prices[0:3]`      | overwrite existing elements                      |
| `read prices[0:10:2]`    | read elements, with numpy-style `start:stop:step`|
| `remove prices[0:2]`     | remove elements and return them                  |
| `get prices`             | look up a variable's type and length             |

## Smart Files

A smart file is the same storage engine without the type system or variables:
one untyped byte array in a file, with the same ACID guarantees. Reach for it
when you want transactional inserts into the middle of a blob and don't need a
schema.

Smart files share the rest of the API: `ns_begin`, `ns_commit`, `ns_rollback`,
`ns_close`, `ns_crash` and `ns_cleanup` all work on an `nsdb_t *` from
`ns_smfile_open`. What they don't share is the query language - a smart file has
no variables, so `ns_execute`, `ns_read`, `ns_write` and plans don't apply.
