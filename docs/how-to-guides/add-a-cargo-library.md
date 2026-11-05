# Adding a new Cargo library

Three steps: create the crate, add it to the workspace, expose it to CMake.

## 1. Create the crate

Rust libraries go under `libs/`, Rust binaries under `apps/`.

```sh
cargo new --lib libs/nsmetrics
```

Point its manifest at the workspace so versions stay in one place:

```toml
# libs/nsmetrics/Cargo.toml
[package]
name = "nsmetrics"
version.workspace = true
edition.workspace = true
license.workspace = true

[dependencies]
```

## 2. Add it to the workspace

The root `Cargo.toml` lists members explicitly - a glob like `libs/*` would try
to read `Cargo.toml` from `libs/numstore` and `libs/pynumstore`, which are C and
Python:

```toml
[workspace]
members = ["libs/nsserver", "libs/nsmetrics", "apps/numstore", "apps/nsclient"]
```

Shared dependency versions belong in `[workspace.dependencies]`, and the crate
picks them up with `foo.workspace = true`.

## 3. Expose it to CMake

Nothing to do. `cmake/Rust.cmake` imports the whole workspace:

```cmake
corrosion_import_crate(MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml)
```

Every workspace member becomes a CMake target named after its Cargo package, so
`nsmetrics` is importable the moment it is a member.

## Verify

```sh
cargo build --workspace
cmake -S . -B build/rust
cmake --build build/rust
```

A plain `rlib` produces no linkable artifact on its own - see
[C calling Rust](c-calling-rust.md) for the `staticlib` setup.

## Watch out for

- Target names. `apps/numstore` keeps its directory name but its Cargo package
  is `numstore-app`, because a `numstore` Cargo target would collide with the
  `numstore` C library target.
- The workspace always builds, so a new crate's compile errors break the CMake
  build too. cibuildwheel is the exception - it stops before the Rust include
  and never needs cargo.
