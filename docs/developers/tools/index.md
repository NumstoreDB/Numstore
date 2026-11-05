# Developer Tools

Every binary the debug build puts in `build/debug/bin/`. The pretty-printers
and the type tools take no database lock, so they are safe to point at a file
another process has open.

| Tool | Usage | What it is for |
|---|---|---|
| `unit_tests` | `unit_tests SEED [FILTER]` | the C test suite; debug builds only |
| `ns_simtest` | `ns_simtest --duration S --seed N` | random operations against a reference model, with crashes |
| `ns_nspprint` | `ns_nspprint FNAME [PGNO...] [DL\|IN]` | pretty-print database pages, whole file or by page number |
| `ns_nsspprint` | `ns_nsspprint FNAME` | pretty-print a smart file |
| `ns_walpprint` | `ns_walpprint FNAME` | pretty-print a write-ahead log, record by record |
| `ns_dlread` | `ns_dlread FNAME PGNO` | dump the raw bytes of a data-list page chain |
| `print_all_vars` | `print_all_vars DBNAME [FILTER]` | list the variables in a database |
| `print_query` | `print_query "QUERY"` | compile a query and print the plan, without running it |
| `print_type` | `print_type "TYPE"` | compile a type string and print its size and layout |
| `resolve_type_ref` | `resolve_type_ref "TYPE" "REF"` | resolve a field reference against a type |

The two below are the ones worth knowing in detail.

## unit_tests

`unit_tests` takes a seed and an optional name filter, so
`./build/debug/bin/unit_tests 123142 wal` runs only the tests whose name
contains `wal`. It exists in debug builds only: its `main` is generated at
configure time by `apps/gen_tests.py`, which scrapes every `TEST(...)` in
`libs/numstore/src/`.

## ns_simtest

`ns_simtest` drives a database with random operations, checks it against a
reference model and crashes it at random:
`./build/debug/bin/ns_simtest --duration 5`.
