---
title: DLREAD
section: 1
header: Numstore Manual
footer: numstore 1.0
date: September 2026
---

# NAME

dlread - dump the contents of a numstore DATA_LIST page chain

# SYNOPSIS

**dlread** *FNAME* *PGNO*

# DESCRIPTION

**dlread** is a low-level diagnostic tool for inspecting a **numstore**(1)
database file on disk. It opens the pager for *FNAME*, walks the chain of
**DATA_LIST** pages starting at page *PGNO*, and writes the raw payload bytes
stored in that chain to standard output.

For each page visited, **dlread** also writes a short diagnostic block to
standard error containing the page number, its **next** and **prev** page
pointers, and the number of payload bytes used on that page (**blen**). This
lets a caller separate the recovered data se to inspect.

*PGNO* : Page number of the first **DATA_LIST** page in the chain to dump. Must
be a valid page number within *FNAME*.

# OUTPUT

Standard output receives the concatenated raw payload bytes of every page in
the chain, with no framing or formatting - this is a binary dump, not text,
unless the underlying data happens to be printable.

Standard error receives one diagnostic block per page, of the form:

    ============================= PGNO
    DATA_LIST
    next: NEXT_PGNO
    prev: PREV_PGNO
    blen: BYTES_USED
    Wrote: BYTES_WRITTEN

*NEXT_PGNO* and *PREV_PGNO* are **PGNO_NULL** when there is no successor or
predecessor page.

# EXIT STATUS

**0** : The tool ran to completion. Note that pager or page-lookup errors
encountered while walking the chain are logged and cause **dlread** to return
early, but currently still result in exit status *0*; check standard error for
a logged error if the output looks truncated.

**-1** : Incorrect number of command-line arguments.

# EXAMPLES

Dump the DATA_LIST chain rooted at page 42 of *mydb* to a file, while watching
the page-walk diagnostics on the terminal:

    dlread mydb 42 > payload.bin

Inspect only the diagnostics, discarding the payload:

    dlread mydb 42 > /dev/null

# BUGS

Pager and page-retrieval failures are logged via the internal error reporting
facility and cause the page walk to stop, but are not currently reflected in
the process exit status.

# SEE ALSO

**numstore**(1)

# AUTHOR

Written by Theo Lincke.

# COPYRIGHT

Copyright 2026 Theo Lincke. Licensed under the Apache License, Version 2.0. See
*http://www.apache.org/licenses/LICENSE-2.0* for details.tdout) from page-level
bookkeeping (stderr), for example by redirecting stdout to a file while leaving
the diagnostics visible on the terminal.

**dlread** follows the **next** pointer of each page until it reaches a page
with no successor, concatenating each page's payload in order. It does not
interpret the payload as any particular **numstore** type (see **numstore**(1)
for the type system) - it simply dumps the bytes that make up the chain,
exactly as stored on disk.

This tool operates directly on the pager layer and bypasses the query engine
entirely. It is intended for debugging and internal inspection, not for normal
data access; use the **numstore**(1) REPL and its **READ** command for that.

# ARGUMENTS

*FNAME* : Path to the numstore database fil(
