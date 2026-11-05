# Make Targets

One `CMakeLists.txt` builds everything, and `make` wraps the cmake invocations
so the flags live in one place:

| Target                 | What it does                                        |
| ---------------------- | --------------------------------------------------- |
| `make debug`           | debug build into `build/debug` (the default target) |
| `make release`         | release build into `build/release`                  |
| `make release-package` | release build, then `cmake --install` into `build/release/package` |
| `make test`            | `test-unit`, then `test-simtest`                    |
| `make test-unit`       | debug build, then `unit_tests` with a random seed   |
| `make test-simtest`    | debug build, then `ns_simtest` for 50 seconds       |
| `make test-python`     | pip install the package, then run `pytest`          |
| `make wheel`           | build a wheel into `dist/` (`make sdist` for a tarball) |
| `make coverage`        | gcovr HTML report into `build/coverage/html`        |
| `make valgrind`        | `unit_tests` and `ns_simtest` under valgrind        |
| `make leaks`           | the same two under macOS `leaks`                    |
| `make format`          | clang-format, gersemi and ruff over the tree        |
| `make clean`           | remove build artifacts                              |

`coverage` needs gcovr, `format` needs clang-format, gersemi and ruff, and
`valgrind` and `leaks` need the tool they're named after. CI runs `make
test-unit` and `make release-package` on every push.

## Building from source

The wheel brings numpy and nothing else. To build the bindings from source
instead - which is what you want if you're working on Numstore itself - you'll
need Python 3.9+, a C11 compiler and CMake 3.20+:

```sh
git clone https://github.com/NumstoreDB/Numstore
cd Numstore
python3 -m venv .venv && . .venv/bin/activate # Optional
pip install .
python samples/sample1_basic.py
```

`pip install .` compiles the C library and the bindings in one step. The sample
creates a database file in your current directory.
