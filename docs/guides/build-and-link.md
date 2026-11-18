Build and link the C library
============================

Numstore is a C11 library with one public header,
`libs/numstore/include/numstore.h`, and a set of static archives that you link
together. It has no dependencies beyond libc, pthreads and libm.

Build
-----

Build the library from the repository:

```sh
git clone https://github.com/NumstoreDB/Numstore
cd Numstore
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

That leaves you with nine archives in `build/release/lib/` and the samples in
`build/release/bin/`. `libns_numstore.a` is the public API, and the rest are the
layers under it. For a debug build, use
`-B build/debug -DCMAKE_BUILD_TYPE=Debug` instead.

Link with pkg-config
--------------------

Install the library, then compile your program against it:

```sh
cmake --install build/release --prefix "$HOME/.local"
export PKG_CONFIG_PATH="$HOME/.local/lib/pkgconfig"
cc -std=c11 myprog.c $(pkg-config --cflags --libs numstore) -o myprog
```

The install puts the header, the nine archives and `numstore.pc` under the
prefix. `pkg-config` gives you the include path and the archives in link order,
plus `-pthread -lm` on Linux.

Link with CMake
---------------

From CMake, point at the checkout and link the exported target. The include
directory and the other archives come with it:

```cmake
cmake_minimum_required(VERSION 3.20)
project(myapp C)

add_subdirectory(Numstore)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE numstore::numstore)
```

If you'd rather link against a system path,
`cmake --install build/release --prefix /usr/local` installs the archives, the
header and a CMake package. A project that uses the installed copy calls
`find_package (numstore)` and links the same `numstore::numstore` target.

Next steps
----------

* [Getting started](../getting-started.md#a-first-program) - your first program.
