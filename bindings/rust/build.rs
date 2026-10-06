use std::path::{Path, PathBuf};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    build_numstore()?;
    Ok(())
}

fn build_numstore() -> Result<(), Box<dyn std::error::Error>> {
    // Root numstore cmake project
    let root = PathBuf::from("../../numstore").canonicalize()?;

    let dst = cmake::Config::new(&root)
        .build_target("numstore.library")
        .build();

    let mut archives = Vec::new();
    find_archives(&dst.join("build"), &mut archives);
    if archives.is_empty() {
        return Err("CMake target numstore.library` produced no static archive".into());
    }

    // Find the archives
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
