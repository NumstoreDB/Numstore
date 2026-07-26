/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
use glob;
use regex::Regex;
use std::env;
use std::fs;
use std::path::Path;
use std::path::PathBuf;

// build.rs
fn main() {
    // Glob source files
    let csrcs = get_files(&["src/*.c", "src/testing/*.c"]);
    let hsrcs = get_files(&["src/*.h", "src/testing/*.h"]);
    let utsrc = Path::new("src/templates/unit_tests.c.in");

    track_files(&[&csrcs, &hsrcs, &vec![utsrc.to_path_buf()]]);

    // Generate C Unit Test Executable
    let unit_test_c = format!("{}/unit_tests.c", env::var("OUT_DIR").unwrap());
    generate_tests(&csrcs, &utsrc.to_path_buf(), &unit_test_c.as_str());

    // Generate the numstore library
    let mut build = cc::Build::new();

    // Add the source files
    build.define("TESTING", "1");
    build.include("src");
    build.std("c11");
    build.file(unit_test_c.as_str());
    for csrc in csrcs {
        build.file(csrc);
    }

    let opts = Options::from_env();
    add_compile_flags(&mut build, &opts);

    build.compile("numstore");
}

fn get_files(patterns: &[&str]) -> Vec<PathBuf> {
    let mut ret = Vec::new();

    for pattern in patterns {
        for entry in glob::glob(pattern).expect("Failed to read glob pattern") {
            match entry {
                Ok(path) => {
                    ret.push(path);
                }
                Err(e) => {
                    eprintln!("glob error: {:?}", e)
                }
            }
        }
    }

    ret
}

fn track_files(files_list: &[&Vec<PathBuf>]) {
    for files in files_list {
        for file in files.iter() {
            println!("cargo:rerun-if-changed={}", file.display());
        }
    }
}

/// Find all TEST(name) macros across files matching `glob_patterns`,
/// render them into `input_template` (replacing %CALLS% / %TEST_COUNT%),
/// and write the result to `output_file`.
fn generate_tests(files: &Vec<PathBuf>, input_template: &PathBuf, output_file: &str) {
    println!("cargo:rerun-if-changed={}", input_template.display());

    let test_pattern = Regex::new(r"TEST\s*\(\s*(\w+)\s*\)").unwrap();
    let mut calls = String::new();
    let mut count = 0;

    for path in files {
        // Read the entire file
        let content = match fs::read_to_string(&path) {
            Ok(c) => c,
            Err(_) => match fs::read(&path) {
                Ok(bytes) => String::from_utf8_lossy(&bytes).into_owned(),
                Err(e) => panic!("Failed to read file {}: {}", path.display(), e),
            },
        };

        // Regex search for TEST pattern
        for cap in test_pattern.captures_iter(&content) {
            let m = cap.get(0).unwrap();
            let name = cap.get(1).unwrap().as_str();
            let line = content[..m.start()].matches('\n').count() + 1;

            calls.push_str(&single_test_template(&path, line, name));
            count += 1;
        }
    }

    if calls.len() == 0 {
        panic!("Error: No tests found.");
    }

    let template = fs::read_to_string(input_template).unwrap_or_else(|e| {
        panic!(
            "Error reading template '{}': {}",
            input_template.display(),
            e
        );
    });

    let out = template
        .replace("%CALLS%", &calls)
        .replace("%TEST_COUNT%", count.to_string().as_str());

    fs::write(output_file, out).unwrap_or_else(|e| {
        panic!("Error writing output '{}': {}", output_file, e);
    });
}

fn single_test_template(filepath: &PathBuf, line: usize, name: &str) -> String {
    format!(
        r#"  //////////////////// {filepath}:{line} START
  if (!filter || strstr("{name}", filter))
  {{
    extern void __test__{name}(void);
    i_log_info("========================= TEST CASE: %s\n", "{name}");
    int prev = test_ret;
    test_ret = 0;
    __test__{name}();
    if (!test_ret)
    {{
      i_log_passed("%s\n", "{name}");
      test_ret = prev;
    }}
    else
    {{
      failed_names[failed++] = "{name}";
    }}
    ntests++;
  }}
  //////////////////// {filepath}:{line} DONE
"#,
        filepath = filepath.display(),
        line = line,
        name = name,
    )
}

/// Mirrors the CMake `option()` block in CMakeLists.txt.
struct Options {
    debug: bool,           // inverse of CMAKE_BUILD_TYPE STREQUAL "Release"
    enable_portable: bool, // ENABLE_PORTABLE
    enable_tests: bool,    // ENABLE_TESTS
    enable_logging: bool,  // ENABLE_LOGGING
    optimize_code: bool,   // OPTIMIZE_CODE
    enable_coverage: bool, // ENABLE_COVERAGE
}

impl Options {
    fn from_env() -> Self {
        let debug = env::var("PROFILE").as_deref() != Ok("release");
        let enable_coverage = env::var("NS_COVERAGE").as_deref() == Ok("1");

        Options {
            debug,
            enable_portable: false,
            enable_tests: true, //debug,
            enable_logging: debug,
            optimize_code: debug,
            enable_coverage,
        }
    }
}

fn add_compile_flags(build: &mut cc::Build, opts: &Options) {
    if !opts.debug {
        build.define("NDEBUG", None);
    }
    if opts.enable_tests {
        build.define("TESTING", None);
    }
    if !opts.enable_logging {
        build.define("NLOG", None);
    }

    let tool = build.get_compiler();
    let is_msvc = tool.is_like_msvc();
    let is_clang = tool.is_like_clang();

    if is_msvc {
        build
            .flag("/W3")
            .flag("/wd4100")
            .flag("/wd4101")
            .flag("/wd4244")
            .flag("/wd4267")
            .flag("/experimental:c11atomics");

        if opts.debug {
            build.flag("/Od").flag("/Zi");
        }

        if !opts.enable_portable {
            build.flag("/arch:AVX2");
        }

        if opts.enable_coverage {
            panic!("Coverage is only supported on GCC/Clang.");
        }
    } else {
        build
            .flag("-Wall")
            .flag("-Wextra")
            .flag("-Werror")
            .flag("-Wshadow")
            .flag("-Wsign-compare")
            .flag("-Wstrict-prototypes")
            .flag("-Wmissing-prototypes")
            .flag("-Wmissing-declarations")
            .flag("-pedantic-errors")
            .flag("-Wno-unused-parameter")
            .flag("-Wno-unused-variable")
            .flag("-Wno-unused-but-set-variable");

        if opts.debug {
            build.flag("-g").flag("-O0");
        } else if opts.optimize_code {
            build.flag("-O3");
        }

        if is_clang {
            build
                .flag_if_supported("-Wno-gnu-zero-variadic-macro-arguments")
                .flag_if_supported("-Wno-static-in-inline");
        }

        if opts.enable_coverage {
            build.flag("--coverage").flag("-fprofile-update=atomic");
            println!("cargo:rustc-link-arg=--coverage");
        }

        if opts.enable_portable {
            match env::var("CARGO_CFG_TARGET_ARCH").as_deref() {
                Ok("x86_64") => {
                    build.flag("-march=x86-64").flag("-mtune=generic");
                }
                Ok("aarch64") => {
                    build.flag("-march=armv8-a").flag("-mtune=generic");
                }
                _ => {}
            }
        } else {
            build.flag("-march=native").flag("-mtune=native");
        }
    }
}
