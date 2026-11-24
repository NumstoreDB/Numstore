// libs/c/numstore-sys/build.rs
fn main() {
    // This is set from CMake - it's the directory of the numstore static library
    let lib = std::env::var("NUMSTORE_LIB_DIR").unwrap();

    // Link with the numstore libraries. A static library comes before the
    // ones it depends on
    println!("cargo:rustc-link-search=native={}", lib);
    for name in ["ns_common"] {
        println!("cargo:rustc-link-lib=static={}", name);
    }

    // Re generate on changed
    println!("cargo:rerun-if-changed=../../c/numstore/include/numstore.h");
    println!("cargo:rerun-if-env-changed=NUMSTORE_LIB_DIR");
}
