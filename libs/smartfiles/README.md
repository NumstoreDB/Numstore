# Smartfiles
--------------

1.0 Synopsis
============

Smart Files is a storage wrapper around Numstore. 
Files have had the same definition for 
50 years: an array of bytes that grows, shrinks, and seeks. 
Smart Files extends that model 
with crash safety, inner mutations, and more.

1.1 Motivation
==============

Standard files have two fundamental problems:

1. **No crash safety.**         - `fwrite` does not guarantee bytes hit disk. 
                                  A crash mid-write leaves the file in an 
                                  unknown state with no path to recovery.

2. **No inner mutations.**      - There is no standard way to insert or 
                                  remove bytes in the middle of a file without 
                                  rewriting everything that follows.

### 1.1.1 What Smart Files Adds

1. **Transactions**             - Modifications go through a write-ahead log. 
                                  Every write commits fully or rolls back cleanly; 
                                  a crash mid-write leaves nothing corrupt.

2. **Inner mutations**          - Insert or remove bytes anywhere in the stream 
                                  in O(log N) time.

1. **Stride access**            - Read, write, and remove at regular intervals 
                                  without manual offset arithmetic.

4. **Multiple named streams**   - Store more than one named byte stream per file 

Written in C with no dependencies. 
Developed on POSIX; 
built (but not yet optimized) for Windows.

---

1.2 Quick Start
===============

```bash
git clone https://github.com/lincketheo/smartfiles.git
cd smartfiles
git submodule update --init
cmake --preset release
cmake --build --preset release
./build/release/samples/sample1_simple   # run a sample
sudo cmake --install ./build/release
```

---

1.2.1 A Sample Application
--------------------------

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
  // NULL second parameter selects the default stream.
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

  // Remove every 3rd element; removed values are copied into read_data
  smfile_premove(f, NULL, read_data, sizeof(uint32_t), 0, 3, 200);
  print_array("Expect: [0, 3, 6, 9, ...]", read_data, 20);

  // Same strided read - stream has changed shape after removal
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

**Compile and run:**

```bash
gcc main.c -o main -lsmartfiles -L/usr/local/lib
./main
```

**Expected output:**

```
Expect: [0, 2, 4, 6, ...]
     0 2 4 6 8 0 2 4 6 8 10 12 14 16 18 20 22 24 26 28
Expect: [0, 3, 6, 9, ...]
     0 3 6 9 2 5 8 11 14 17 20 23 26 29 32 35 38 41 44 47
Expect: [1, 4, 7, 0, ...]
     1 4 7 0 3 6 9 12 15 18 21.14 27 30 33 36 39 42 45 48
Expect: [0, 1, 2, 3, ...]
     0 1.1 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19
```

---

1.2.2 Step-by-Step Walkthrough
------------------------------

* Step 1                        - Insert at position 0.
                                  Write the full 200,000-element array at
                                  byte offset 0. stream = [0, 1, 2, 3, ...].

* Step 2                        - Insert in the middle.
                                  Insert another 200,000-byte chunk at byte
                                  offset 40 (element index 10). Everything
                                  from index 10 onwards shifts right, producing
                                  [0, 1, ..., 9, 0, 1, 2, ..., 9, 10, 11, ...]
                                  at the join.

* Step 3                        - Strided read (stride = 2).
                                  smfile_pread reads every 2nd uint32_t from
                                  offset 0, returning [0, 2, 4, 6, 8, 0, 2, ...].
                                  The 0 at index 5 marks where the inserted
                                  block begins.

* Step 4                        - Strided remove (stride = 3).
                                  smfile_premove pulls every 3rd uint32_t and
                                  copies the removed values into read_data.
                                  Gaps close; remaining elements shift down.

* Step 5                        - Strided read again (stride = 2).
                                  Same read as Step 3, but the stream is shorter.
                                  After removal sequence = [1, 4, 7, 0, ...].

* Step 6                        - Strided write (stride = 2).
                                  smfile_pwrite overwrites every 2nd element
                                  with values from data (0, 1, 2, 3, ...).

* Step 7                        - Final read.
                                  The last strided read returns [0, 1, 2, 3, ...]
                                  even-indexed slots hold the clean sequence
                                  written in Step 6.
