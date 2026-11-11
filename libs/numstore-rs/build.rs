use std::env;

// libs/numstore-sys/build.rs
fn main() {
    // Where numstore is defined
    let numstore = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../numstore");

    // This is set from CMake - it's the directory of the numstore static library
    let lib = std::env::var("NUMSTORE_LIB_DIR").unwrap();

    // Link with the numstore library
    println!("cargo:rustc-link-search=native={}", lib);
    println!("cargo:rustc-link-lib=static=numstore");

    // Re generate on changed
    println!("cargo:rerun-if-changed=../numstore/src/nscore/nsdb/ns_nsdb.h");
    println!("cargo:rerun-if-changed=../nslang/include/nslang.h");
    println!("cargo:rerun-if-changed={}", lib);

    bindgen::Builder::default()
        .header(numstore.join("include/numstore.h").to_str().unwrap())
        .clang_arg(format!("-I{}", numstore.join("src").to_str().unwrap()))
        .clang_arg(format!(
            "-I{}",
            numstore.join("../nslang/include").to_str().unwrap()
        ))
        .generate()
        .unwrap()
        .write_to_file(std::path::Path::new(&std::env::var("OUT_DIR").unwrap()).join("bindings.rs"))
        .unwrap();
}
