# Numstore

**An embedded ACID database for Numpy arrays.**

Numstore is a database for arrays. It is an ACID database meant to store
contiguous arrays of numerical values. Full introduction:
[What is Numstore?](docs/concepts/what-is-numstore.md).

## Features

- **Single File.** The whole database lives in a single file. There is no
  server - see `libs/numstore/include/numstore.h`
- **ACID.** Every insert, write and remove either happens completely or not at
  all, even if the process crashes partway through. Transactions are strictly
  serializable - one is open at a time - and crash recovery uses ARIES over a
  unique rope variant of the B+Tree to index into arrays. Concurrent readers and
  writers are in development.
- **Arrays.** Numstore stores images, sensor streams, tick data and any fixed
  layout of numbers you can describe as a type.
- **No dependencies.** Written in C11 with first class Python bindings that
  return numpy arrays.

## Quick start

Pynumstore is the easiest way to get started:

```sh
pip install pynumstore
```

```python
import numpy as np
import pynumstore as ns

with ns.Database("prices.db") as db:
    db.execute("create prices f64")
    db.write("insert prices 0 3", np.array([1.5, 2.25, 3.75]))

    with db.begin() as tx:              # commits on success, rolls back on error
        tx.write("insert prices 3 2", [4.0, 5.0])

    print(db.read("read prices[0:]"))   # [1.5  2.25 3.75 4.   5.  ]
    print(db.read("read prices[0::2]")) # [1.5  3.75 5.  ]
```

## Layout

```text
libs/numstore/      one C static library; include/numstore.h is the only public header
libs/pynumstore/    the Python package and its CPython extension (tests/ is not shipped)
libs/nsserver/      Rust: the gRPC service (see docs/rpc.md)
apps/               C debug tools, plus the Rust server and client binaries
samples/            runnable samples
docs/               documentation
```

Dependencies run one way: **`pynumstore` -> `numstore` -> Rust crates.** C
consumes Rust; Rust crates never call back into numstore. See
[C calling Rust](docs/how-to-guides/c-calling-rust.md).

The Rust workspace always builds, so building this tree needs a Rust
toolchain. The wheels are the exception - a `scikit-build-core` build stops at
`pynumstore` and needs no cargo -
[what would change when wheels need Rust](docs/how-to-guides/wheels-with-rust.md).

## Pick your path

| I want to... | Go to |
|---|---|
| Try it in five minutes | [Getting started](docs/getting-started.md) |
| Read the Python reference | [Python API](docs/integrator/reference/python-api.md) |
| Read the C reference | [C API](docs/integrator/reference/c-api.md) |
| Build Numstore from source | [Build debug and release](docs/developers/reference/make-targets.md) |
| Add a Rust crate, or cross the C/Rust boundary | [Guides](docs/how-to-guides/add-a-cargo-library.md) |
| Talk to numstore over gRPC | [The numstore RPC](docs/rpc.md) |
| Start from C | [C: First database](docs/integrator/tutorials/c-first-database.md) |

Every page is indexed in [docs/index.md](docs/index.md). Release notes are in
[CHANGELOG.md](CHANGELOG.md).

## AI Usage Policy

I use AI the way I use a language server: as a tool, not a co-author. AI usage
is fine, but not for heavy lifting.

Things I ask AI to do:

- Add edge-case test scenarios to existing unit tests (reviewed before
  committing).
- Review an algorithm I've written and flag anything that looks wrong.
- Write formatting scripts, CI/CD glue, and other boilerplate I could write
  myself but would rather not.

Things I don't ask AI to do:

- Implement features.
- Delete or replace code I've written.
- Read a paper and implement the algorithm.

In practice, AI is useful for ideation, code review, and generating mundane
code I'll immediately refactor. Every algorithm in this codebase was written by
me.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Apache 2.0. See [LICENSE](LICENSE).
