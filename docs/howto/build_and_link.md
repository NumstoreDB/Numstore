Build and Link the C Library
============================

Numstore is a C11 library. It has one public header,
`libs/numstore/include/numstore.h`, and a set of static archives. You link
the archives together. There are no dependencies other than libc, pthreads and
libm.

Build
-----

Build the library from the repository:

```sh
git clone https://github.com/NumstoreDB/Numstore
cd Numstore
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

The build makes nine archives in `build/release/lib/` and the samples in
`build/release/bin/`. `libns_numstore.a` is the public API. The other archives
are the layers below it.

For a debug build, use `-B build/debug -DCMAKE_BUILD_TYPE=Debug`.

Link with pkg-config
--------------------

Install the library. Then compile your program:

```sh
cmake --install build/release --prefix "$HOME/.local"
export PKG_CONFIG_PATH="$HOME/.local/lib/pkgconfig"
cc -std=c11 myprog.c $(pkg-config --cflags --libs numstore) -o myprog
```

The install puts the header, the nine archives and `numstore.pc` in the prefix.
`pkg-config` gives the include path and the archives in link order. On Linux,
it also gives `-pthread -lm`.

Link with CMake
---------------

In CMake, add the checkout and link the exported target. The include directory
and the other archives come with it:

```cmake
cmake_minimum_required(VERSION 3.20)
project(myapp C)

add_subdirectory(Numstore)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE numstore::numstore)
```

To link against a system path, run
`cmake --install build/release --prefix /usr/local`. It installs the archives,
the header and a CMake package. A project that uses the installed copy calls
`find_package (numstore)`. It links the same `numstore::numstore` target.

Next steps
----------

* [Getting started](../GETTING_STARTED.md#a-first-program) has a first program.
