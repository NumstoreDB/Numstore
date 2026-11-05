# Documentation Plan

The documentation follows a four-audience tree: `concepts/` for the model,
`integrator/` for embedding Numstore, `developers/` for working on it, and
building), how-to guides (one task), reference (look it up) and concept pages.

52 pages are written. 68 are planned: the template for each
existed but had nothing in it, so the page was removed and listed below rather
than shipped empty. Each entry records what the page is for and where its
content should come from.

## Tree shape

```text
docs/
  concepts/
    concurrency.md                            written
    data-input-output.md                      written
    data-model.md                             written
    durability-and-recovery.md                written
    glossary.md                               planned
    query-language/
      insert.md                               written
      overview.md                             written
      read.md                                 written
      remove.md                               written
      variables.md                            written
      write.md                                written
    storage-model.md                          planned
    transactions.md                           written
    types/
      combining-types.md                      written
      overview.md                             written
      primitives.md                           written
      strict-arrays.md                        written
      structs.md                              written
      unions.md                               written
    vs-other-databases.md                     written
    what-is-numstore.md                       written
  developers/
    concepts/
      architecture.md                         planned
      locking-and-latching.md                 planned
      memory-allocators.md                    planned
      os-abstraction-and-fault-injection.md   planned
      pager/
        aries.md                              planned
        buffer_pool.md                        planned
        overview.md                           planned
        txns.md                               planned
      query-compiler.md                       planned
      rope/
        insert.md                             planned
        overview.md                           planned
        read.md                               planned
        remove.md                             planned
        seek.md                               planned
        write.md                              planned
      testing-strategy.md                     planned
      type-internals.md                       planned
      vars/
        delete.md                             planned
        get.md                                planned
        hash_map.md                           planned
        overview.md                           planned
      wal.md                                  planned
    how-to/
      add-a-python-binding-method.md          planned
      add-a-regression-test.md                planned
      add-os-specific-code.md                 planned
      build-wheels.md                         planned
      check-coverage.md                       planned
      cut-a-release.md                        planned
      format-code.md                          planned
      increase-log-level.md                   planned
      preview-the-docs.md                     planned
      reproduce-a-simtest-failure.md          planned
      run-simtest.md                          planned
      run-unit-tests-by-seed-and-filter.md    planned
      run-valgrind-or-leaks.md                planned
    reference/
      ci-workflows.md                         planned
      cmake-options.md                        planned
      code-style.md                           planned
      make-targets.md                         written
      on-disk-format.md                       planned
    tools/
      ns_dlread.md                            planned
      ns_nspprint.md                          planned
      ns_nsspprint.md                         planned
      ns_walpprint.md                         planned
      print_all_vars.md                       planned
      print_query.md                          planned
      print_type.md                           planned
      resolve_type_ref.md                     planned
    tutorials/
      build-and-run-tests.md                  planned
      debug-a-corrupt-database.md             planned
      trace-an-insert-end-to-end.md           planned
      write-a-unit-test.md                    planned
  documentation_plan.md                       written
  getting-started.md                          written
  how-to-guides/
    add-a-cargo-library.md                    written
    c-calling-rust.md                         written
    rust-calling-c.md                         written
    wheels-with-rust.md                       written
  integrator/
    how-to/
      c-handle-errors.md                      written
      c-read-into-a-numstore-buffer.md        written
      c-read-into-your-own-buffer.md          written
      c-recover-after-a-crash.md              written
      c-store-packed-structs.md               written
      c-use-prepared-plans.md                 written
      c-use-smart-files.md                    written
      python-handle-errors.md                 written
      python-read-into-preallocated-array.md  written
      python-reuse-a-query-plan.md            written
      python-use-threads-safely.md            written
      validate-queries-and-types.md           planned
    reference/
      c-api.md                                written
      errors.md                               planned
      limits-and-platforms.md                 planned
      python-api.md                           written
      query-grammar.md                        planned
      type-grammar.md                         planned
    troubleshooting.md                        written
    tutorials/
      c-first-database.md                     written
      c-link-with-cmake.md                    written
      python-first-database.md                planned
      python-structs-and-numpy-dtypes.md      planned
      python-surviving-a-crash.md             planned
      python-transactions-and-rollback.md     planned
      type-design-exercises.md                written
  releases/
    v1.5.0.md                                 written
    v1.5.1.md                                 written
  rpc.md                                      written
```

## Pages still to write

### Concepts

| Page | Kind | What it should cover | Content comes from |
|---|---|---|---|
| `concepts/glossary.md` | reference | Glossary: Definitions: variable, element, plan, page, WAL, rope, and so on. | - |
| `concepts/storage-model.md` | concept | Storage Model: Single file, .wal sidecar, rope-indexed arrays (high level only). | - |

### Integrator guide

| Page | Kind | What it should cover | Content comes from |
|---|---|---|---|
| `integrator/how-to/validate-queries-and-types.md` | how-to | Validate Queries and Types Before Using Them: Use print_query and print_type. | - |
| `integrator/reference/errors.md` | reference | Errors: Error codes and messages with causes and fixes. | - |
| `integrator/reference/limits-and-platforms.md` | reference | Limits and Supported Platforms: Sizes, limits, OS and architecture matrix. | .github/workflows/ci.yml |
| `integrator/reference/query-grammar.md` | reference | Query Grammar: Formal grammar for queries. | - |
| `integrator/reference/type-grammar.md` | reference | Type Grammar: Formal grammar for type strings. | - |
| `integrator/tutorials/python-first-database.md` | tutorial | Python: Your First Database: Create, insert, read with pynumstore. | samples/sample1_basic.py |
| `integrator/tutorials/python-structs-and-numpy-dtypes.md` | tutorial | Python: Structs and NumPy dtypes: Map composite types to NumPy dtypes. | samples/sample2_datatypes.py |
| `integrator/tutorials/python-surviving-a-crash.md` | tutorial | Python: Surviving a Crash: Simulate a crash and recover. | samples/sample4_crash.py |
| `integrator/tutorials/python-transactions-and-rollback.md` | tutorial | Python: Transactions and Rollback: Commit on success, roll back on error. | samples/sample3_transactions.py |

### Developer guide

| Page | Kind | What it should cover | Content comes from |
|---|---|---|---|
| `developers/concepts/architecture.md` | concept | Architecture: Layers: core, nscore, numstore.c, bindings. | `libs/numstore/src/ns_pynumstore.c`, `libs/numstore/src/numstore.c`, `include/numstore.h` |
| `developers/concepts/locking-and-latching.md` | concept | Locking and Latching: Lock table, latches and the group-reader lock. | `libs/numstore/src/core/ns_gr_lock.c`, `libs/numstore/src/core/ns_latch.c`, `libs/numstore/src/nscore/lock_table/ns_lock_table.c` |
| `developers/concepts/memory-allocators.md` | concept | Memory Allocators: Arena and slab allocators and the malloc wrapper. | `libs/numstore/src/core/ns_arena_alloc.c`, `libs/numstore/src/core/ns_slab_alloc.c`, `libs/numstore/src/core/os/ns_malloc.c` |
| `developers/concepts/os-abstraction-and-fault-injection.md` | concept | OS Abstraction and Fault Injection: posix / windows / faulty OS layers used by simtest. | `libs/numstore/src/core/os/faulty/ns_faulty_os.c`, `libs/numstore/src/core/os/ns_os.h` |
| `developers/concepts/pager/aries.md` | concept | Pager: ARIES Recovery: Logging, checkpoints and the analysis / redo / undo phases. | `libs/numstore/src/nscore/dpg_table/ns_dirty_page_table.c`, `libs/numstore/src/nscore/pager/ns_pgr_checkpoint.c`, `libs/numstore/src/nscore/pager/ns_pgr_logging.c`, `libs/numstore/src/nscore/pager/ns_pgr_recover.c`, `libs/numstore/src/nscore/testing/ns_aries_tests.c` |
| `developers/concepts/pager/buffer_pool.md` | concept | Pager: Buffer Pool: Fixing, evicting, flushing and releasing pages. | `libs/numstore/src/nscore/pager/ns_pgr_evict.c`, `libs/numstore/src/nscore/pager/ns_pgr_flush.c`, `libs/numstore/src/nscore/pager/ns_pgr_get.c`, `libs/numstore/src/nscore/pager/ns_pgr_get_writable.c`, `libs/numstore/src/nscore/pager/ns_pgr_release.c`, `libs/numstore/src/nscore/pager/ns_pgr_unfix.c` |
| `developers/concepts/pager/overview.md` | concept | Pager Overview: Responsibilities of the pager and how pages move through it. | `libs/numstore/src/nscore/disk_pager/ns_file_pager.c`, `libs/numstore/src/nscore/pager/ns_pager.c` |
| `developers/concepts/pager/txns.md` | concept | Pager: Transactions: Begin, commit, rollback and the transaction table. | `libs/numstore/src/nscore/pager/ns_pgr_begin_txn.c`, `libs/numstore/src/nscore/pager/ns_pgr_cancel.c`, `libs/numstore/src/nscore/pager/ns_pgr_commit.c`, `libs/numstore/src/nscore/pager/ns_pgr_rollback.c`, `libs/numstore/src/nscore/txn_table/ns_txn_table.c` |
| `developers/concepts/query-compiler.md` | concept | Query Compiler: Lexer and parsers for queries, types, subtypes, type refs and strides. | `libs/numstore/src/nscore/compiler/ns_lexer.c`, `libs/numstore/src/nscore/compiler/parsers/ns_parse_query.c`, `libs/numstore/src/nscore/compiler/parsers/ns_parse_type.c` |
| `developers/concepts/rope/insert.md` | concept | Rope: Insert: How insert splits, rebalances and updates counts. | `libs/numstore/src/nscore/algorithms/rope/ns_rope_insert.c`, `libs/numstore/src/nscore/page/ns_page_data_list.c`, `libs/numstore/src/nscore/page/ns_page_inner_node.c` |
| `developers/concepts/rope/overview.md` | concept | Rope Overview: The rope variant of the B+Tree that indexes into arrays. | `libs/numstore/src/nscore/algorithms/rope/ns_rope_algorithms.h`, `libs/numstore/src/nscore/page/ns_page_data_list.c`, `libs/numstore/src/nscore/page/ns_page_inner_node.c` |
| `developers/concepts/rope/read.md` | concept | Rope: Read: How a strided read walks the tree. | `libs/numstore/src/nscore/algorithms/rope/ns_rope_read.c`, `libs/numstore/src/nscore/page/ns_page_data_list.c`, `libs/numstore/src/nscore/page/ns_page_inner_node.c` |
| `developers/concepts/rope/remove.md` | concept | Rope: Remove: How remove merges, rebalances and releases pages. | `libs/numstore/src/nscore/algorithms/rope/ns_rope_remove.c`, `libs/numstore/src/nscore/page/ns_page_data_list.c`, `libs/numstore/src/nscore/page/ns_page_inner_node.c` |
| `developers/concepts/rope/seek.md` | concept | Rope: Seek: How a byte offset is resolved to a data-list page. | `libs/numstore/src/nscore/algorithms/rope/ns_rope_seek.c`, `libs/numstore/src/nscore/page/ns_page_data_list.c`, `libs/numstore/src/nscore/page/ns_page_inner_node.c` |
| `developers/concepts/rope/write.md` | concept | Rope: Write: How an in-place overwrite is applied. | `libs/numstore/src/nscore/algorithms/rope/ns_rope_write.c`, `libs/numstore/src/nscore/page/ns_page_data_list.c`, `libs/numstore/src/nscore/page/ns_page_inner_node.c` |
| `developers/concepts/testing-strategy.md` | concept | Testing Strategy: Unit tests, regressions, simulation, pytest and how they fit together. | `apps/gen_tests.py` |
| `developers/concepts/type-internals.md` | concept | Type Internals: In-memory representation of types, sarrays, structs, unions and type refs. | `libs/numstore/src/nscore/types/ns_sarray_t.c`, `libs/numstore/src/nscore/types/ns_struct_t.c`, `libs/numstore/src/nscore/types/ns_type_ref.c`, `libs/numstore/src/nscore/types/ns_types.c` |
| `developers/concepts/vars/delete.md` | concept | Variables: Delete: Deleting a variable and reclaiming its pages. | `libs/numstore/src/nscore/algorithms/numstore/ns_numstore_delete.c`, `libs/numstore/src/nscore/algorithms/var/ns_var_delete.c` |
| `developers/concepts/vars/get.md` | concept | Variables: Get: Resolving a name to a variable (and get-or-create). | `libs/numstore/src/nscore/algorithms/var/ns_find_var_page.c`, `libs/numstore/src/nscore/algorithms/var/ns_var_get.c`, `libs/numstore/src/nscore/algorithms/var/ns_var_get_or_create.c` |
| `developers/concepts/vars/hash_map.md` | concept | Variables: Hash Map: The on-disk variable hash map and its pages. | `libs/numstore/src/nscore/algorithms/var/ns_init_var_hash_map.c`, `libs/numstore/src/nscore/page/ns_page_var_hash_page.c`, `libs/numstore/src/nscore/page/ns_page_var_page.c`, `libs/numstore/src/nscore/page/ns_page_var_tail.c` |
| `developers/concepts/vars/overview.md` | concept | Variables Overview: How variables are stored and looked up. | `libs/numstore/src/nscore/algorithms/var/ns_var_algorithms.h`, `libs/numstore/src/nscore/variables/ns_variables.c` |
| `developers/concepts/wal.md` | concept | Write-Ahead Log: WAL records, streams and the file format. | `libs/numstore/src/nscore/wal/ns_wal.c`, `libs/numstore/src/nscore/wal/ns_wal_istream.c`, `libs/numstore/src/nscore/wal/ns_wal_ostream.c`, `libs/numstore/src/nscore/wal/ns_wal_record.c` |
| `developers/how-to/add-a-python-binding-method.md` | how-to | Add a Python Binding Method: Touch ns_pynumstore.c, the .pyi stub and pytest. | - |
| `developers/how-to/add-a-regression-test.md` | how-to | Add a Regression Test: Follow the 000N_name.c convention. | - |
| `developers/how-to/add-os-specific-code.md` | how-to | Add OS-Specific Code: posix / windows / faulty layers. | - |
| `developers/how-to/build-wheels.md` | how-to | Build Wheels: make wheel / cibuildwheel. | .github/workflows/wheels.yml |
| `developers/how-to/check-coverage.md` | how-to | Check Coverage: make coverage with gcovr. | the make target table in docs/developers/reference/make-targets.md (README.md's Developing section moved there) |
| `developers/how-to/cut-a-release.md` | how-to | Cut a Release: version.txt, tags, wheels, tarball, Pages. | .github/workflows/ |
| `developers/how-to/format-code.md` | how-to | Format Code: make format: clang-format, gersemi, ruff. | the make target table in docs/developers/reference/make-targets.md (README.md's Developing section moved there) |
| `developers/how-to/increase-log-level.md` | how-to | Increase the Log Level: Set I_LOG_LEVEL so LOG_INFO tools print. | - |
| `developers/how-to/preview-the-docs.md` | how-to | Preview the Docs: make docs and pandoc setup. | .github/workflows/pages.yml; docs/CMakeLists.txt |
| `developers/how-to/reproduce-a-simtest-failure.md` | how-to | Reproduce a Simtest Failure: Replay a failing seed and minimize. | - |
| `developers/how-to/run-simtest.md` | how-to | Run the Simulation Test: ns_simtest durations, seeds and fault rates. | the make target table in docs/developers/reference/make-targets.md (README.md's Developing section moved there) |
| `developers/how-to/run-unit-tests-by-seed-and-filter.md` | how-to | Run Unit Tests by Seed and Filter: ./unit_tests SEED NAME. | the make target table in docs/developers/reference/make-targets.md (README.md's Developing section moved there) |
| `developers/how-to/run-valgrind-or-leaks.md` | how-to | Run Valgrind or Leaks: make valgrind / make leaks. | the make target table in docs/developers/reference/make-targets.md (README.md's Developing section moved there) |
| `developers/reference/ci-workflows.md` | reference | CI Workflows: ci, wheels, source-tarball, pages. | .github/workflows/ |
| `developers/reference/cmake-options.md` | reference | CMake Options: Cache variables and build types. | CMakeLists.txt |
| `developers/reference/code-style.md` | reference | Code Style: Naming (ns_ prefix), error handling, license header, formatting. | .clang-format |
| `developers/reference/on-disk-format.md` | reference | On-Disk Format: Page types, headers, WAL record types. | - |
| `developers/reference/repo-map.md` | reference | Repository Map: Directory to responsibility. | - |
| `developers/tools/ns_dlread.md` | tool | ns_dlread: Dump raw bytes from a data-list page chain. | `apps/ns_dlread.c` |
| `developers/tools/ns_nspprint.md` | tool | ns_nspprint: Print page metadata through the validating pager. | `apps/ns_nspprint.c` |
| `developers/tools/ns_nsspprint.md` | tool | ns_nsspprint: Print raw pages with no validation. | `apps/ns_nsspprint.c` |
| `developers/tools/ns_walpprint.md` | tool | ns_walpprint: Print every record header in a write-ahead log. | `apps/ns_walpprint.c` |
| `developers/tools/print_all_vars.md` | tool | print_all_vars: List the variables in a database. | `apps/print_all_vars.c` |
| `developers/tools/print_query.md` | tool | print_query: Compile a query string and print the result. | `apps/print_query.c` |
| `developers/tools/print_type.md` | tool | print_type: Compile a type string and print the result. | `apps/print_type.c` |
| `developers/tools/resolve_type_ref.md` | tool | resolve_type_ref: Resolve a type ref against a type. | `apps/resolve_type_ref.c` |
| `developers/tutorials/build-and-run-tests.md` | tutorial | Build and Run the Tests: Clone, build debug, run unit tests and simtest. | the make target table in docs/developers/reference/make-targets.md (README.md's Developing section moved there) |
| `developers/tutorials/debug-a-corrupt-database.md` | tutorial | Debug a Corrupt Database: Use the page and WAL tools to find the damage. | - |
| `developers/tutorials/trace-an-insert-end-to-end.md` | tutorial | Trace an Insert End to End: Follow one insert from query string to disk. | - |
| `developers/tutorials/write-a-unit-test.md` | tutorial | Write a Unit Test: Add a TEST(...) and run it by name and seed. | - |


## Referenced from the code

Headers carry a `// For more information, see docs/...` pointer above the area
each page describes, so a planned page already has a caller waiting for it.
`grep -rn "For more information, see docs/" libs` lists them.
