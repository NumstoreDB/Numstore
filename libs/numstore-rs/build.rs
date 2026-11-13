use std::env;

// libs/numstore-sys/build.rs
fn main() {
    // Where numstore is defined
    let numstore = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../numstore");

    // This is set from CMake - it's the directory of the numstore static library
    let lib = std::env::var("NUMSTORE_LIB_DIR").unwrap();

    // Link with the numstore libraries. A static library comes before the
    // ones it depends on
    println!("cargo:rustc-link-search=native={}", lib);
    for name in [
        "ns_numstore",
        "ns_nsdb",
        "ns_algorithms",
        "ns_nscore",
        "ns_disk_wal",
        "ns_disk_pager",
        "ns_types",
        "ns_page",
        "ns_common",
    ] {
        println!("cargo:rustc-link-lib=static={}", name);
    }

    // Re generate on changed
    println!("cargo:rerun-if-changed=../numstore/include/numstore.h");
    println!("cargo:rerun-if-env-changed=NUMSTORE_LIB_DIR");

    bindgen::Builder::default()
        .header(numstore.join("include/numstore.h").to_str().unwrap())
        .generate()
        .unwrap()
        .write_to_file(std::path::Path::new(&std::env::var("OUT_DIR").unwrap()).join("bindings.rs"))
        .unwrap();
}
