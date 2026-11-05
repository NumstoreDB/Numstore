# C: Link with CMake

Build the library out of the repository:

```sh
git clone https://github.com/NumstoreDB/Numstore
cd Numstore
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

That leaves you with `build/release/lib/libnumstore.a` and the samples in
`build/release/bin/`. To compile a program of your own against it:

```sh
cc -std=c11 -I include myprog.c build/release/lib/libnumstore.a -o myprog
```

On Linux add `-pthread -lm`; on macOS and Windows neither is needed. For a
debug build, use `-B build/debug -DCMAKE_BUILD_TYPE=Debug` instead.

From CMake, point at the checkout and link the exported target. The include
directory comes with it:

```cmake
cmake_minimum_required(VERSION 3.20)
project(myapp C)

add_subdirectory(Numstore)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE numstore::numstore)
```

`cmake --install build/release --prefix /usr/local` installs the archive and the
header if you would rather link against a system path.
