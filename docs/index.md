# Numstore Documentation

Numstore is a single-file ACID database for arrays. A variable has a name, a
type and an array of elements; you work with it through seven short queries
rather than SQL. Transactions are strictly serializable and a crash cannot
leave half a transaction behind.

- **New here, want it running in five minutes** -
  [Getting started](getting-started.md)
- **Want to understand the model before writing code** -
  [What is Numstore?](concepts/what-is-numstore.md), then
  [the query language](concepts/query-language/overview.md) and
  [the type system](concepts/types/overview.md)
- **Embedding it in Python** -
  [Python API](integrator/reference/python-api.md), and
  [how-to guides](integrator/how-to/python-handle-errors.md) for the awkward parts
- **Embedding it in C** -
  [C API](integrator/reference/c-api.md) and
  [linking with CMake](integrator/tutorials/c-link-with-cmake.md)
- **Working on Numstore itself** -
  [make targets](developers/reference/make-targets.md),
  [the tools](developers/tools/index.md), and
  [how-to guides](how-to-guides/add-a-cargo-library.md) for the C/Rust boundary

Everything else is in the tree beside this page. What is not written yet is
listed in the [documentation plan](documentation_plan.md).
