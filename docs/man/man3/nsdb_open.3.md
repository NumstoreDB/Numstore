---
title: NSDB_OPEN
section: 3
header: Numstore Programmer's Manual
footer: numstore 1.0
date: September 2026
---

# NAME

numstore_open, numstore_close, numstore_cleanup, numstore_crash - open, close, and manage the lifecycle of a numstore database

# SYNOPSIS

**#include <numstore.h>**

*numstore_t \**
**numstore_open**(*const char \*path*);

*int*
**numstore_close**(*numstore_t \*ns*);

*int*
**numstore_cleanup**(*const char \*path*);

*int*
**numstore_crash**(*numstore_t \*ns*);

# DESCRIPTION

These four functions manage the on-disk lifecycle of a
**numstore**(1) database: creating or opening a handle to one,
closing it cleanly, deleting it entirely, and simulating an ungraceful
crash for testing purposes.

**numstore_open**()
: Opens the database located at *path* using the library's default
  memory allocator and default filesystem implementation, and
  returns a handle for use with the rest of the numstore API
  (**numstore_fexecute**(3), **numstore_begin**(3), and related functions).
  It is a convenience wrapper around **numstore_open_with_resources**(3);
  programs that need a custom allocator or filesystem should call
  that function directly instead.

**numstore_close**()
: Closes *ns*, flushing any pending state to disk and releasing all
  resources associated with the handle. After a successful call,
  *ns* must not be used again. Every handle returned by
  **numstore_open**() should eventually be passed to **numstore_close**().

**numstore_cleanup**()
: Deletes all on-disk state associated with the database at *path*,
  including any of its supporting files (for example, its
  write-ahead log). Unlike **numstore_close**(), this does not operate on
  an open handle - it takes a path directly, and is typically used to
  reset a database to a clean, nonexistent state before opening a
  fresh one, such as at the start of a test.

**numstore_crash**()
: Simulates an ungraceful crash of the database referenced by *ns*,
  as opposed to the graceful shutdown performed by
  **numstore_close**(). This is used to exercise the database's crash
  recovery path: the handle should not be used again after this
  call except to reopen it. It is intended for testing recovery
  behavior, not for normal use.

# RETURN VALUE

**numstore_open**() returns a valid, non-NULL *numstore_t \** handle on
success, or *NULL* on failure. Because no handle exists yet when this
call fails, there is no **error** object available to inspect for
this call specifically (see **numstore_strerror**(3)); consult the
process log for diagnostic detail.

**numstore_close**(), **numstore_cleanup**(), and **numstore_crash**() return
*0* on success and a non-zero value on failure. For **numstore_close**()
and **numstore_crash**(), the specific error can be retrieved with
**numstore_strerror**(3) or **numstore_perror**(3) before the handle is
discarded.

# NOTES

**numstore_cleanup**() acts on a path, not a handle, and can safely be
called whether or not a database currently exists at that path - it
is the usual first step before a fresh **numstore_open**() call, both in
tests and in any code path that wants to start from a known-empty
state.

After **numstore_crash**(), the correct way to resume using the database
is to call **numstore_open**() again on the same path, exercising
whatever recovery logic the crash was meant to test - not to
continue using the old handle.

# EXAMPLES

    if (numstore_cleanup ("mydb") != 0) {
        /* handle cleanup failure */
    }

    numstore_t *db = numstore_open ("mydb");
    if (db == NULL) {
        /* handle open failure */
    }

    /* ... use db ... */

    if (numstore_close (db) != 0) {
        /* handle close failure */
    }

Simulating a crash and recovering:

    numstore_crash (db);
    db = numstore_open ("mydb");   /* triggers recovery */

# SEE ALSO

**numstore**(1), **numstore_open_with_resources**(3),
**numstore_strerror**(3), **numstore_begin**(3), **numstore_fexecute**(3)

# AUTHOR

Written by Theo Lincke.

# COPYRIGHT

Copyright 2026 Theo Lincke. Licensed under the Apache License,
Version 2.0. See *http://www.apache.org/licenses/LICENSE-2.0* for
details.
