# SmartFiles Performance: Inner Inserts

I'll be starting a performance series on Numstore (specifically the SmartFiles
pattern) and why SmartFiles can far surpass regular vanilla reads and writes
in some contexts for file I/O.

You can run all of these performance tests using the tests in
[the Numstore Workbench Repo](https://github.com/NumstoreDB/Numstore-Workbench).

For this series, I want to stay truthful and humble about what Numstore can
and can't do. To stay truthful, I'm breaking down performance into small,
self-contained use cases. I'll explore each use case fully and deeply, and
talk about where Numstore shines and where it shouldn't be used. I'll never
intentionally hide a domain of performance in order to conceal the limitations
of Numstore.

I did all these performance analyses on an 8-core Intel i9 Dell Inspiron 2019
laptop. I slowed down my system by turning off any daemons and heavy running
processes. I ran all tests 3 times overnight, wrapped by Python subprocess.

## Definitions

**SmartFiles** — Numstore's file abstraction that makes inner inserts and
removals first-class operations (something the OS does not natively support)
while providing ACID guarantees and write-ahead logging under the hood.

**Inner Insert** — An inner insert shifts existing bytes to the right and
places new data at a given offset, growing the file. This is distinct from an
inner write, which overwrites bytes in place without changing the file's
length.

**FALLOC_FL_INSERT_RANGE** — A Linux `fallocate(2)` flag that instructs the
kernel to insert a hole at a given offset, shifting subsequent data right.
Requires XFS (Linux 4.1+) or ext4 (Linux 4.2+), and mandates that both the
offset and insertion length be multiples of the filesystem block size.

## Abstract

There were three main takeaways from this performance analysis of Numstore:

1. Generally, with naive file I/O, the longer your file, the longer it will
   take to insert data into the middle of that file. This is not the case with
   SmartFiles. Inserting a fixed-size insert buffer of length *n* into the
   middle of a 100-byte file is comparable to inserting the same fixed-size
   insert buffer into a 20 GiB file — where regular file I/O would have
   exploded. Therefore, for workloads characterized by high data volume and
   frequent interior inserts, SmartFiles offers a substantial advantage.

1. Generally, as the length of the fixed-size insert buffer increases (as in
   case 1), SmartFiles doesn't offer any performance benefit over raw file
   I/O, and is around 3× slower due to durability overhead and logging.
   Workloads that predominantly write large, contiguous payloads to interior
   offsets should not expect a performance gain from SmartFiles.

1. SmartFiles is functionally comparable to `fallocate(2)` with
   `FALLOC_FL_INSERT_RANGE`, but with fewer constraints. Benchmarks show
   SmartFiles to be more consistently performant at higher file sizes.
   `FALLOC_FL_INSERT_RANGE` imposes significant restrictions in practice: it
   requires an ext4 filesystem and mandates that both the insertion length and
   offset be integer multiples of the filesystem block size. SmartFiles
   carries no such constraints, making it more portable and easier to reason
   about in general-purpose contexts.

![A plot of time vs insertion size](assets/p6_bar_time_vs_file_size_log.png)

*Figure 0 — Time vs. file size with a fixed offset and file size (both
relatively small). As file size increases, SmartFiles takes relatively the
same amount of time to insert data into the interior of the file
(logarithmically scaling to the file size).*

## Inner Inserts

For this first performance analysis, I'll talk about inner mutations —
specifically inserting data into the middle of a file. Later I'll talk about
inner removals, which completes the "inner mutation" operations analysis.

### Definition: Inner Insert

Say I have a file where each byte is just its location on disk. Byte 0 has
value 0, byte 1 has value 1:

```
[ 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 ]
```

Normal files provide first-class support for *inner writes*. So let's say I
want to write from index 1 to 3 with new data `[ 9 | 10 | 11 ]`. After a
traditional "write" system call, that would look like this:

```
[ 0 | 9 | 10 | 11 | 4 | 5 | 6 | 7 | 8 ]
```

Notice how 1, 2, and 3 were overwritten by 9, 10, and 11. This is what I'll
call an "inner write."

What if I want to insert data at index 1? This is actually a really common use
case. So now I want to insert the same data (`[ 9 | 10 | 11 ]`) into position
1 of the file like this:

```
[ 0 | 9 | 10 | 11 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 ]
```

This is what I will call an "inner insert" (as opposed to an "inner write").

### The Problem Space

For the sake of measuring performance, an inner insert has the following
parameters:

- `fsize` — Original File Size. The size of the file before we make an insert.
- `isize` — Desired Data Insert Length. The size of the data we want to insert
  into the file.
- `offst` — Desired Data Offset. The location in the original file we want to
  insert the data into.

One more term to help frame the problem:

- `tail` — The remainder of the file from `file[offst ... fsize]`.

### Solving it Naively

Let's try to solve this without SmartFiles. In general, the pattern looks like
this:

1. Read the "tail" of the file into memory, effectively splitting the file
   into two segments — before and after our offset.
1. Write our data to the end of this truncated file.
1. Write the tail back to the end of the file.

Visualized:

```
file = [ 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 ]

1. Read in the tail:
     file = [ 0 ]
     tail = [ 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 ]

2. Write our data to the end of the truncated file:
     file = [ 0 | 9 | 10 | 11 ]
     tail = [ 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 ]

3. Write the tail back to the end of the file:
     file = [ 0 | 9 | 10 | 11 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 ]
```

Unbuffered implementation:

```c
{
    pread (fd, tail, tail_size, ofst);
    pwrite (fd, tail, tail_size, ofst + insize);
    pwrite (fd, insert, insize, ofst);
    i_fsync (fd);
}
```

Note that this method has unbounded memory overhead. The file can be any size,
and no matter what, we need to "touch" all the data within the tail of the
file in some way or another. To address this, let's try buffering.

Buffering is what `FILE* fp = fopen(...)` gives you over plain POSIX
`int fd = open(...)`. Buffering limits memory overhead by maintaining a
fixed-size buffer in memory. With a buffer of 4096 bytes, we never need to
hold more than 4096 bytes in memory at once. However, we still "touch" all the
bytes to the right of our insertion point. As the file grows without bound, we
always need to touch the right half of our file in some way, unless we have an
OS-level primitive for it. That's not ideal.

Buffered implementation:

```c
{
  {
    u64 remaining = fsize - ofst;
    u64 read_pos  = fsize;

    // Read in chunks
    while (remaining > 0)
    {
      const u64 chunk = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;
      read_pos -= chunk;

      i_pread_all_expect (&fp, buffer, chunk, read_pos, &e);
      i_pwrite_all (&fp, buffer, chunk, read_pos + insize, &e);

      remaining -= chunk;
    }
  }
  i_pwrite_all (&fp, insert, insize, ofst, &e);
  i_fsync (&fp, &e);
}
```

### FALLOC_FL_INSERT_RANGE: Solving it Better on Linux

If you're running Linux 4.1 or later with XFS, or Linux 4.2 or later with
ext4, you can call `fallocate(2)` with `FALLOC_FL_INSERT_RANGE`. This is
fairly restricted, though.

From the [man page](https://man7.org/linux/man-pages/man2/fallocate.2.html):

> FALLOC_FL_INSERT_RANGE requires filesystem support. Filesystems that support
> this operation include XFS (since Linux 4.1) and ext4 (since Linux 4.2).

On the system I tested (ext4), both the insertion length and offset must be a
multiple of the filesystem block size. That's a significant constraint for a
reusable program.

Note: `FALLOC_FL_COLLAPSE_RANGE` is the fallocate equivalent to Numstore's
inner remove operation.

## Performance

### Preamble: Why Numstore Is More Than Just Fast

Before talking about performance, I want to note that raw speed isn't the
whole picture — but Numstore still dwarfs a normal system-call-based inner
insert in certain use cases.

The statement:

> Numstore is a lot faster at inner mutations

...fails to capture several things Numstore does on top of being fast. It's
more like:

> Numstore is a lot faster at inner mutations
>
> AND
>
> 1. Numstore is ACID — unlike plain files.
>
>    - You can never write 10 bytes if you requested to write 100 bytes.
>    - If you pull the plug while writing data, Numstore reboots and returns
>      to a consistent state.
>
> 1. Numstore has no extra memory overhead.
>
>    - You never need to care about the tail of your file when doing an inner
>      insert.
>
> 1. Numstore uses less code. Compare the naive implementations above to
>    Numstore: `smfile_insert (file, insert, ofst, insize);`
>
>    - Inner inserts are first-class citizens in Numstore.

Points 1–3 are really significant and I don't want to gloss over them, but
this post is about performance, so let's talk speed.

### Data Capturing

For the following methods of interior insertion:

1. Unbuffered naive file I/O
1. Buffered naive file I/O
1. `fallocate` with `FALLOC_FL_INSERT_RANGE`
1. SmartFiles

**Parameters:**

- **File Size (KiB)** — Size of the original file before the insert.
- **Offset (KiB)** — Where in the file we want to insert data.
- **Insert Size (KiB)** — Size of the buffer we want to insert.
- **Chunk Size (KiB)** — For buffered I/O only; kept constant at system page
  size throughout these results.

**Results:**

- **Time (ms)** — Wall-clock time to execute the operation.

### Round 1: Small File, Large Insert

The first round keeps numbers fairly small:

| Parameter | Value | | ----------- | -------- | | File Size | 100 KiB | |
Offset | 4 KiB | | Insert Size | 9768 KiB |

| Method | Time (ms) | | ---------- | --------- | | Unbuffered | 34.102 | |
Buffered | 21.034 | | Fallocate | 20.587 | | SmartFiles | 114.990 |

This result isn't very meaningful on its own. SmartFiles performs far worse
than the other methods here. The file is relatively small (100 KiB) and the
insertion size is very large (9768 KiB), so *the bottleneck is writing the
insertion data*, not rebalancing the persisted data.

SmartFiles doesn't offer much for small files with large inserts, because it
contains a lot of interior machinery for fault tolerance. Analytically, it
writes 3× as much data — both an undo and a redo for the write-ahead log, plus
the flushed page to disk on the eviction path.

![A plot of time vs insertion size](assets/p2_time_vs_insert_size_linear.png)

*Figure 1 — Time vs. insertion size with a fixed offset and file size (both
relatively small). As insertion size increases, SmartFiles balloons at a
higher rate than naive file I/O — because SmartFiles has substantial interior
machinery for writing a contiguous array of bytes, whereas a plain write is a
single system call.*

> **Take Away**
>
> Short writes into a small database aren't the workload SmartFiles is
> designed for. If your workload is 100-byte files with 10-byte inner
> insertions, don't use Numstore. Simple as that.

### Round 2: Large File, Small Insert

Now let's crank in the other direction — instead of increasing the insertion
size, we increase the file size:

| Parameter | Value | | ----------- | ------ | | File Size | 16 GiB | | Offset
| 4 KiB | | Insert Size | 4 KiB |

| Method | Time (ms) | | ---------- | ---------- | | Buffered | 293,000.00 | |
Fallocate | 2,461.00 | | SmartFiles | 1.903 |

For an already-large dataset (16 GiB), inserting 4 KiB at offset 4 KiB is very
low cost for SmartFiles. For naive file I/O, time balloons dramatically.
Compared to fallocate, you might expect fallocate to be faster, but it tends
to grow at larger file sizes as well. If anyone has ideas why that might be,
I'm happy to adjust my code and regenerate the results.

In general, SmartFiles is very fast when inserting data into an already-large
dataset.

![A linear plot of file size vs time to insert](assets/p1_time_vs_file_size_linear.png)

*Figure 2 — A linear plot of file size (x axis) vs. time to insert a
fixed-size buffer at a fixed offset.*

![A logarithmic plot of file size vs time to insert](assets/p1_time_vs_file_size_log.png)

*Figure 3 — A logarithmic plot of the same data. SmartFiles remains relatively
constant as file size increases.*

![A heat map of time vs file size vs insert size](assets/p5_heatmap_file_x_insert_log.png)

*Figure 4 — A heat map where x and y represent file size and insertion size.
Naive I/O grows as both x and y increase; SmartFiles only grows as y
increases.*

As the y axis grows (larger insertion sizes), all three methods take longer —
a longer insertion means a longer `write` system call, which is expected. The
notable feature is that as the x axis grows (larger files), naive file I/O
grows proportionally, but SmartFiles remains relatively stable.

> **Take Away**
>
> For large databases (on the order of gigabytes), interior inserts do not
> scale in SmartFiles — they stay relatively level. This workload
> traditionally blows up naive file I/O, and appears to blow up fallocate as
> well.

## Where Is All the Time Going?

A flame graph is a useful tool for understanding which methods are consuming
the most time within a routine. It doesn't give wall-clock measurements, but
it breaks down the relative cost of each call.

![Flamegraph for the buffered file I/O implementation](assets/buffered_flamegraph.png)

*Figure 5 — Flame graph for the buffered file I/O implementation. Roughly
equal time is spent between the `fsync`, `pread`, and `pwrite` calls.*

![Flamegraph for the unbuffered file I/O implementation](assets/unbuffered_flamegraph.png)

*Figure 6 — Flame graph for the unbuffered file I/O implementation. The same
story as Figure 5.*

![Flamegraph for the SmartFiles timed portion](assets/smartfiles_flamegraph.png)

*Figure 7 — Flame graph for the SmartFiles timed portion. A large portion of
time is spent inside `pgr_new` — we're constantly creating new pages and
therefore reading the free-space map frequently (something I intend to
optimize). Significant time is also spent in `pgr_release_with_log`, which
writes data to the WAL and triggers a WAL flush.*

The buffered and unbuffered flame graphs make sense: we spend a lot of time
reading and writing because we must read the tail of the dataset, then write
our data plus that tail back. The `fsync` ensures data is durable on disk.

The SmartFiles flame graph has some interesting properties. Here are a few
unintended bottlenecks I discovered and plan to fix:

1. A lot of time is spent calculating checksums in `walos_write_all` and
   `pgr_flush_unsafe`. When I tried a hardware-accelerated instruction, this
   time dropped noticeably. The next version of Numstore will optimize this
   path.
1. A lot of time is spent reading the free-space map in `pgr_get_writable`.
   This is an artifact of the pager insert pattern. A basic LRU caching
   mechanism failed to trigger on long-running inserts. This is an
   inconsistency and will be fixed in the next release.

The main takeaway from the SmartFiles flame graph is that the dominant system
call is writing to the WAL, not flushing pages to non-volatile storage.
SmartFiles has a bottleneck tied to a single write call, rather than both a
read and a write.
