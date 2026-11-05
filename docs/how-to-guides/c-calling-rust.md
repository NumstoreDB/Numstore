# C calling Rust

This is the direction the repo actually uses: `pynumstore -> numstore -> Rust`.
C consumes Rust; Rust never calls back into numstore.

Three pieces: a crate built as `staticlib`, a C header generated with
`cbindgen`, and `target_link_libraries` from CMake.

## 1. A staticlib crate

```sh
cargo new --lib libs/nschecksum
```

```toml
# libs/nschecksum/Cargo.toml
[package]
name = "nschecksum"
version.workspace = true
edition.workspace = true

[lib]
crate-type = ["staticlib"]

[build-dependencies]
cbindgen = "0.29"
```

```rust
// libs/nschecksum/src/lib.rs
/// # Safety
/// `data` must point to at least `len` readable bytes.
#[no_mangle]
pub unsafe extern "C" fn ns_crc32_rs(data: *const u8, len: usize) -> u32 {
    let bytes = std::slice::from_raw_parts(data, len);
    bytes.iter().fold(0u32, |acc, b| acc.rotate_left(1) ^ u32::from(*b))
}
```

## 2. Generate the header into the crate's include/

```rust
// libs/nschecksum/build.rs
fn main() {
    let dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
    let out = std::path::Path::new(&dir).join("include/nschecksum.h");
    std::fs::create_dir_all(out.parent().unwrap()).unwrap();
    cbindgen::generate(&dir).unwrap().write_to_file(&out);
    println!("cargo:rerun-if-changed=src/lib.rs");
}
```

```toml
# libs/nschecksum/cbindgen.toml
language = "C"
include_guard = "NSCHECKSUM_H"
```

## 3. Link it from CMake

```cmake
# after include(cmake/Rust.cmake)
target_include_directories(numstore
    PRIVATE ${CMAKE_SOURCE_DIR}/libs/nschecksum/include)
target_link_libraries(numstore PUBLIC nschecksum)
```

Then from C:

```c
#include "nschecksum.h"

uint32_t sum = ns_crc32_rs (buf, len);
```

```sh
cmake -S . -B build/rust && cmake --build build/rust
```

## The duplicate-symbol pitfall

**Each Rust `staticlib` bundles its own copy of the Rust standard library.**
Link two of them into one binary and the linker sees `std` twice: duplicate
symbols on some platforms, bloat on all of them.

So do not give every Rust crate `crate-type = ["staticlib"]`. Keep them plain
`rlib`s and add **one** aggregator staticlib that re-exports them:

```toml
# libs/nsffi/Cargo.toml  - the only staticlib in the workspace
[lib]
crate-type = ["staticlib"]

[dependencies]
nschecksum = { path = "../nschecksum" }   # rlib
nsmetrics  = { path = "../nsmetrics" }    # rlib
```

```rust
// libs/nsffi/src/lib.rs
pub use nschecksum::*;
pub use nsmetrics::*;
```

C links `nsffi` and nothing else. One copy of `std`, one header to generate,
and the rlibs stay ordinary Rust libraries that other Rust code can use.

## Watch out for

- `#[no_mangle]` and `extern "C"` on everything crossing the boundary, and
  `#[repr(C)]` on every shared struct.
- A panic across the FFI boundary is undefined behaviour. Catch it at the edge
  (`std::panic::catch_unwind`) or set `panic = "abort"`.
- The generated header is a build product checked into `include/`. Regenerate it
  when the ABI changes, and review the diff.
