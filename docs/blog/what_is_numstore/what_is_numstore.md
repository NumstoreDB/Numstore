What is Numstore?
=================

Numstore is a database for arrays. What do I mean arrays? 


## Definitions

**Contiguous** — Numstore's file abstraction that makes inner inserts and
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

