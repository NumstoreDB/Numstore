#!/usr/bin/env python3

import subprocess
import itertools

program = r"build\debug\bin\Debug\big_file_inner_insert.exe"

file_sizes      =   [100_000, 500_000, 1_000_000, 5_000_000, 10_000_000, 100_000_000, 1_000_000_000]
offsets         =   [100, 1_000, 10_000, 100_000, 1_000_000, 10_000_000, 100_000_000, 1_000_000_000]
insert_sizes    =   [1, 100, 1_000, 10_000, 100_000, 1_000_000, 10_000_000, 100_000_000, 1_000_000_000]
chunk_size     =    4096

first = True

for fsize, ofst, insize in itertools.product(file_sizes, offsets, insert_sizes):

    if ofst >= fsize:
        continue

    args = [program, str(fsize), str(ofst), str(insize), str(chunk_size)]

    if first:
        args.append("--verbose")
        first = False

    result = subprocess.run(args, capture_output=False)
