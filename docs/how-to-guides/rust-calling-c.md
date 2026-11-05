# Rust calling a C library

A `-sys` crate holds the `extern "C"` declarations and the link directive;
anything above it is safe Rust.

The dependency direction in this repo is `pynumstore -> numstore -> Rust`, so
this is the direction that is **not** used in production code. It is documented
because the pattern comes up for tooling.

## 1. Create the sys crate

```sh
cargo new --lib libs/numstore-sys
```

```toml
# libs/numstore-sys/Cargo.toml
[package]
name = "numstore-sys"
version.workspace = true
edition.workspace = true

[build-dependencies]
bindgen = "0.72"
```

## 2. Declare the ABI

Hand-written is fine for a handful of functions:

```rust
// libs/numstore-sys/src/lib.rs
use std::ffi::{c_char, c_int};

#[repr(C)]
pub struct nsdb_t {
    _opaque: [u8; 0],
}

#[link(name = "numstore")]
extern "C" {
    pub fn ns_open(path: *const c_char) -> *mut nsdb_t;
    pub fn ns_execute(db: *mut nsdb_t, txn: *mut c_void, query: *const c_char) -> c_int;
    pub fn ns_close(db: *mut nsdb_t) -> c_int;
}
```

For the whole header, generate it instead:

```rust
// libs/numstore-sys/build.rs
fn main() {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../numstore");
    println!("cargo:rustc-link-lib=static=numstore");
    println!("cargo:rerun-if-changed=../numstore/include/numstore.h");

    bindgen::Builder::default()
        .header(root.join("include/numstore.h").to_str().unwrap())
        .generate()
        .unwrap()
        .write_to_file(
            std::path::Path::new(&std::env::var("OUT_DIR").unwrap()).join("bindings.rs"),
        )
        .unwrap();
}
```

## 3. Tell the linker where the archive is

`libnumstore.a` is a CMake build product, so its path is only known to CMake.
Pass it in through an environment variable that `build.rs` reads:

```cmake
corrosion_set_env_vars(numstore-sys
    NUMSTORE_LIB_DIR=$<TARGET_FILE_DIR:numstore>)
```

```rust
println!("cargo:rustc-link-search=native={}",
         std::env::var("NUMSTORE_LIB_DIR").unwrap());
```

And make the ordering explicit so the archive exists first:

```cmake
add_dependencies(cargo-build_numstore-sys numstore)
```

## Minimal working example

```rust
// libs/numstore-sys/examples/open.rs
use std::ffi::CString;

fn main() {
    let path = CString::new("guide.db").unwrap();
    unsafe {
        let db = numstore_sys::ns_open(path.as_ptr());
        assert!(!db.is_null());
        numstore_sys::ns_close(db);
    }
    println!("opened and closed guide.db");
}
```

```sh
cmake -S . -B build/rust && cmake --build build/rust
cargo run -p numstore-sys --example open
```

## Watch out for

- The C library needs `Threads::Threads`, and `m` on glibc. Add
  `cargo:rustc-link-lib=m` on those targets or link the example through CMake.
- `numstore.h` is C11 with atomics; bindgen needs `-std=c11` in
  `.clang_arg("-std=c11")` on some toolchains.
