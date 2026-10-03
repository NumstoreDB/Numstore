//! Build script for nsserver.
//!
//! 1. Builds the `numstore.library.private` target from ../numstore with CMake
//!    and links every static archive it produced.
//! 2. Compiles the .proto files in proto/ into Rust (client + server code).

use std::path::{Path, PathBuf};

/// CMake target to build. Only this target (and what it depends on) is
/// compiled, so the little exes in ../numstore are skipped.
const CMAKE_TARGET: &str = "numstore.library";

/// System libraries libnumstore needs. A static archive doesn't carry its own
/// dependencies, so list them here (e.g. "m", "pthread").
const SYSTEM_LIBS: &[&str] = &[];

const PROTOS: &[&str] = &["proto/numstore/v1/numstore.proto"];

fn main() -> Result<(), Box<dyn std::error::Error>> {
    build_numstore()?;
    build_protos()?;
    Ok(())
}

fn build_numstore() -> Result<(), Box<dyn std::error::Error>> {
    let root = PathBuf::from("../numstore").canonicalize()?;

    let dst = cmake::Config::new(&root)
        .build_target(CMAKE_TARGET)
        // Keep optional parts of the C project out of the Rust build.
        .define("NUMSTORE_BUILD_PYTHON", "OFF")
        .build();

    // We built exactly one target (plus its dependencies), so every static
    // archive in the CMake build tree belongs to it. Searching for them means
    // this keeps working whatever OUTPUT_NAME or output directory CMake uses.
    let mut archives = Vec::new();
    find_archives(&dst.join("build"), &mut archives);
    if archives.is_empty() {
        return Err(format!(
            "CMake target `{CMAKE_TARGET}` produced no static archive (.a/.lib). \
             Is it a STATIC library?"
        )
        .into());
    }

    for archive in &archives {
        let dir = archive.parent().unwrap();
        let file = archive.file_name().unwrap().to_str().unwrap();
        let name = file
            .strip_prefix("lib")
            .unwrap_or(file)
            .trim_end_matches(".a")
            .trim_end_matches(".lib");
        println!("cargo:rustc-link-search=native={}", dir.display());
        println!("cargo:rustc-link-lib=static={name}");
    }
    for lib in SYSTEM_LIBS {
        println!("cargo:rustc-link-lib={lib}");
    }

    // Rebuild the C side when it changes.
    for p in ["src", "include", "CMakeLists.txt"] {
        println!("cargo:rerun-if-changed={}", root.join(p).display());
    }
    Ok(())
}

fn find_archives(dir: &Path, out: &mut Vec<PathBuf>) {
    let Ok(entries) = std::fs::read_dir(dir) else {
        return;
    };
    for entry in entries.flatten() {
        let path = entry.path();
        if path.is_dir() {
            // Skip CMake's own scratch dirs (compiler checks etc.)
            if path.file_name().is_some_and(|n| n == "CMakeFiles") {
                continue;
            }
            find_archives(&path, out);
        } else if path.extension().is_some_and(|e| e == "a" || e == "lib") {
            out.push(path);
        }
    }
}

fn build_protos() -> Result<(), Box<dyn std::error::Error>> {
    // Use $PROTOC if set (e.g. a Homebrew install), otherwise the bundled one,
    // so nobody needs protoc installed to build.
    if std::env::var_os("PROTOC").is_none() {
        // SAFETY: build scripts are single-threaded at this point.
        unsafe { std::env::set_var("PROTOC", protoc_bin_vendored::protoc_bin_path()?) };
    }

    tonic_prost_build::configure()
        .build_server(true)
        .build_client(true)
        .compile_protos(PROTOS, &["proto"])?;

    println!("cargo:rerun-if-changed=proto");
    Ok(())
}
