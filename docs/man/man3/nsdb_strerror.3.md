---
title: NSDB_STRERROR
section: 3
header: Numstore Programmer's Manual
footer: numstore 1.0
date: September 2026
---

# NAME

numstore_strerror, numstore_perror - describe the last error recorded on a numstore handle

# SYNOPSIS

**#include <numstore.h>**

*const char \**
**numstore_strerror**(*numstore_t \*ns*);

*int*
**numstore_perror**(*numstore_t \*ns*, *const char \*prefix*);

# DESCRIPTION

These functions describe the most recent error recorded on the
database handle *ns*, in the style of **strerror**(3) and
**perror**(3). Each operation performed through *ns* - for example
**numstore_fexecute**(3) - records its own error state on the handle at
the start of the call, so the description returned reflects the
outcome of the most recently completed operation on *ns*, not any
earlier one.

**numstore_strerror**()
: Returns a pointer to a human-readable, statically- or
  internally-owned string describing the last error recorded on
  *ns*. The caller must not free the returned pointer.

**numstore_perror**()
: Prints a human-readable description of the last error recorded on
  *ns* to standard error, prefixed with *prefix* followed by a colon
  and a space, in the same style as **perror**(3).

# RETURN VALUE

**numstore_strerror**() returns a non-NULL pointer to a string describing
the last recorded error.

**numstore_perror**() returns *0* on success, or a non-zero value if the
message could not be written.

# NOTES

Because error state lives on the handle and is reset at the start of
each operation, call **numstore_strerror**() or **numstore_perror**()
immediately after the failing call, before issuing any further
operation on the same handle - including another **numstore_fexecute**(3)
call made only to check status.

# EXAMPLES

    if (numstore_fexecute (db, NULL, "create foo u32", NULL) != 0) {
        numstore_perror (db, "create foo");
    }

    if (numstore_close (db) != 0) {
        fprintf (stderr, "close failed: %s\n", numstore_strerror (db));
    }

# SEE ALSO

**numstore**(1), **numstore_fexecute**(3), **strerror**(3), **perror**(3)

# AUTHOR

Written by Theo Lincke.

# COPYRIGHT

Copyright 2026 Theo Lincke. Licensed under the Apache License,
Version 2.0. See *http://www.apache.org/licenses/LICENSE-2.0* for
details.
