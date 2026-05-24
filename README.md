<p align="center">
  <img src="docs/logo.png" alt="NumStore Logo" width="200"/>
</p>

# Numstore

## Smart Files 

Smart files is a sub project of numstore 

Files have had the same definition for 50 years: an array of bytes that grows, shrinks, and seeks. Every programming language builds on top of this model. Smart Files extends it with two fixes to longstanding problems, plus two new capabilities.

**Standard files have two fundamental problems:**

1. **No crash safety.** `fwrite` does not guarantee bytes hit disk. A crash mid-write leaves your file in an unknown state with no path to recovery.
2. **No inner mutations.** There is no standard way to insert or remove bytes in the middle of a file without rewriting everything that follows.

**Smart Files solves both, and adds two more features:**

1. **Transactions.** Modifications go through a write-ahead log - every write commits fully or rolls back cleanly. A crash mid-write leaves nothing corrupt.
2. **Inner mutations.** Insert or remove bytes anywhere in the stream in O(log N) time.
3. **Stride access.** Read, write, and remove at regular intervals without manual offset arithmetic.
4. **Multiple named streams.** Store more than one named byte stream per file - no separate file handles, no embedded database.

Written in C with no dependencies. Developed on POSIX - built (but maybe not optimized yet) for windows. 

---

## Quick Start

```bash
git clone https://github.com/lincketheo/smartfiles.git
cd smartfiles
git submodule update --init
cmake --preset release
cmake --build --preset release
./build/release/samples/sample1_simple   # run a sample
sudo cmake --install ./build/release
```

### Write a sample app

```c
#include "smfile.h"

#include <stddef.h>
#include <stdint.h>

void print_array(const char* prefix, uint32_t* arr, int len)
{
  printf("%s\n", prefix);
  printf("     ");
  for (int i = 0; i < len; ++i) {
    printf("%d ", arr[i]);
  }
  printf("\n");
}

int main()
{
  // NULL second parameter is the stream name. NULL selects the default stream.
  // Pass any const char* to name it.
  smfile_t* f = smfile_open("example");

  uint32_t data[200000];
  for (int i = 0; i < 200000; ++i) {
    data[i] = i;
  }

  // Write the array at byte offset 0
  smfile_pinsert(f, NULL, data, 0, sizeof(data));

  // Insert another chunk at byte offset 40 (element index 10)
  smfile_pinsert(f, NULL, data, 10 * sizeof(uint32_t), 200000);

  // Read every 2nd uint32_t starting at offset 0
  uint32_t read_data[200];
  smfile_pread(f, NULL, read_data, sizeof(uint32_t), 0, 2, 200);
  print_array("Expect: [0, 2, 4, 6, ...]", read_data, 20);

  // Remove every 3rd element; the removed values are copied into read_data
  smfile_premove(f, NULL, read_data, sizeof(uint32_t), 0, 3, 200);
  print_array("Expect: [0, 3, 6, 9, ...]", read_data, 20);

  // Same strided read - the stream has changed shape after the removal
  smfile_pread(f, NULL, read_data, sizeof(uint32_t), 0, 2, 200);
  print_array("Expect: [1, 4, 7, 0, ...]", read_data, 20);

  // Overwrite every 2nd element with values from data
  smfile_pwrite(f, NULL, data, sizeof(uint32_t), 0, 2, 200);

  // Final read - even-indexed slots now hold a clean sequence
  smfile_pread(f, NULL, read_data, sizeof(uint32_t), 0, 2, 200);
  print_array("Expect: [0, 1, 2, 3, ...]", read_data, 20);

  smfile_close(f);
  return 0;
}
```

### Compile and run

```bash
gcc main.c -o main -lsmartfiles -L/usr/local/lib
./main
```

Expected output:

```
Expect: [0, 2, 4, 6, ...]
     0 2 4 6 8 0 2 4 6 8 10 12 14 16 18 20 22 24 26 28
Expect: [0, 3, 6, 9, ...]
     0 3 6 9 2 5 8 11 14 17 20 23 26 29 32 35 38 41 44 47
Expect: [1, 4, 7, 0, ...]
     1 4 7 0 3 6 9 12 15 18 21 24 27 30 33 36 39 42 45 48
Expect: [0, 1, 2, 3, ...]
     0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19
```

### What's happening step by step

**Step 1 - insert at position 0.**
We write the full 200,000-element array at byte offset 0. The stream now holds `[0, 1, 2, 3, ...]`.

**Step 2 - insert in the middle.**
We insert another 200,000-byte chunk at byte offset 40 (element index 10). Everything from index 10 onwards shifts right. At the join you get `[0, 1, ..., 9, 0, 1, 2, ..., 9, 10, 11, ...]`.

**Step 3 - strided read (stride=2).**
`smfile_pread` reads every 2nd `uint32_t` from offset 0, returning `[0, 2, 4, 6, 8, 0, 2, ...]`. The `0` at index 5 is where the inserted block begins.

**Step 4 - strided remove (stride=3).**
`smfile_premove` pulls every 3rd `uint32_t` and copies the removed values into `read_data`. The gaps close up; remaining elements shift down.

**Step 5 - strided read again (stride=2).**
Same read as step 3, but the stream is shorter now. After the removal the sequence starts `[1, 4, 7, 0, ...]`.

**Step 6 - strided write (stride=2).**
`smfile_pwrite` overwrites every 2nd element with values from `data` (0, 1, 2, 3, ...).

**Step 7 - final read.**
The last strided read returns `[0, 1, 2, 3, ...]` - the even-indexed slots hold the clean sequence written in step 6.

---

## Project Structure

Public headers are in `include/`. Source code is in `lib/`. The `thirdparty/c_specx` directory holds reusable C utilities I've extracted for use across projects - all my own code, no external dependencies.

`lib/` is organized by function:

- `algorithms` - rope algorithms and database traversal
- `aries` - rollback and crash recovery logic
- `dpgt` - dirty page table
- `lockt` - lock table
- `os_pager` - single-file pager that reads pages from disk
- `pager` - buffer pool initialization, page reads, and WAL entry writes
- `pages` - page type definitions
- `testing` - test-specific utilities
- `txns` - transaction table and transaction logic
- `wal` - write-ahead log

---

## How I Use AI

I use AI the way I use a language server: as a tool, not a co-author.

**Things I ask AI to do:**

- Add edge-case test scenarios to an existing unit test (I review every one before committing)
- Review an algorithm I've written and flag anything that looks wrong
- Write formatting scripts, CI/CD glue, and other boilerplate I could write myself but would rather not

**Things I don't ask AI to do:**

- Implement features
- Delete or replace code I've written
- Read a paper and implement the algorithm

In practice, AI is useful for ideation, code review, and generating mundane code I'll immediately refactor. For any algorithm going into this codebase, I write it, and I own it.

The `CLAUDE.md` file is there because if AI-assisted code ever does land here, I want it to follow consistent standards. It was inspired by [Karpathy Inspired Claude Code](https://github.com/forrestchang/andrej-karpathy-skills).

If you're skeptical that a database engine can be written without leaning on AI - fair. Read through the code and make up your own mind.

---

## Contributing

File a ticket on GitHub for bugs, feature requests, or questions. Pull requests are welcome - see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines. Windows CI/CD support is an open and approachable first contribution.

## License

Apache 2.0. See [LICENSE](LICENSE).
