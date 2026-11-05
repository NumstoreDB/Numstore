# Wheels with Rust

Today no Rust is in the wheel. A `scikit-build-core` build returns from the
root `CMakeLists.txt` right after `pynumstore`, before `cmake/Rust.cmake` is
included, so `pip install .` and cibuildwheel need no cargo and the
cibuildwheel config mentions Rust nowhere.

When a wheel does need Rust, this is what changes.

## 1. A toolchain in the build environment

```toml
[tool.cibuildwheel]
before-all = "curl -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal"
environment = { PATH = "$HOME/.cargo/bin:$PATH" }
```

manylinux and musllinux images have no rustup preinstalled. On Windows the
installer is different (`rustup-init.exe`), so that platform usually needs its
own `before-all`.

## 2. Reach the Rust include on the wheel path

The `if(SKBUILD)` branch in the root `CMakeLists.txt` returns before the Rust
section. A wheel that needs Rust has to include it on that path too:

```cmake
if(SKBUILD)
    add_subdirectory(libs/pynumstore)
    include(cmake/Rust.cmake)
    return()
endif()
```

## 3. Check the targets that are easy to get wrong

| Target | What to check |
|---|---|
| manylinux | the image's glibc against the Rust target; `aarch64` builds run under emulation and are slow |
| musllinux | needs the `*-unknown-linux-musl` target installed |
| macOS | `MACOSX_DEPLOYMENT_TARGET` must match between cargo and CMake; universal2 needs both arch targets |
| Windows | MSVC vs gnu toolchain has to match how the C library was built |

## 4. Keep Cargo.lock honest

The lock file is committed, so wheel builds are reproducible. `--locked` in CI
turns a stale lock into a failure rather than a silent update.
