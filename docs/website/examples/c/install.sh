#!/usr/bin/env bash

# Build and install the library
git clone https://github.com/NumstoreDB/Numstore
cd Numstore
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
cmake --install build/release --prefix "$HOME/.local"

# Save the code below as main.c, then compile and run it
export PKG_CONFIG_PATH="$HOME/.local/lib/pkgconfig"
cc -std=c11 main.c $(pkg-config --cflags --libs numstore) -o main
./main
