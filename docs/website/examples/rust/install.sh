#!/usr/bin/env bash

# Build the C libraries
git clone https://github.com/NumstoreDB/Numstore
cd Numstore
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release

# Run the example. It is in libs/rust/numstore-rs/examples/quickstart.rs
NUMSTORE_LIB_DIR=$PWD/build/release/lib cargo run --release --example quickstart
