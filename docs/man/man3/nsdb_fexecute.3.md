---
title: NSDB_FEXECUTE
section: 3
header: Numstore Programmer's Manual
footer: numstore 1.0
date: September 2026
---

# NAME

numstore_fexecute, numstore_fexecute_malloc - compile and execute a printf-style numstore query

# SYNOPSIS

**#include <numstore.h>**

*sb_size*
**numstore_fexecute**(*numstore_t \*ns*, *ns_txn_t \*txn*, *const char \*query_fmt*, *void \*data*, *...*);

*void \**
**numstore_fexecute_malloc**(*numstore_t \*ns*, *ns_txn_t \*txn*, *const char \*query_fmt*, *void \*data*, *...*);

# DESCRIPTION

Both functions format *query_fmt* as a **printf**(3)-style template
using any trailing variadic arguments, compile the resulting string
as a **numstore**(1) query, and execute it against *ns*.

*txn* is an optional explicit transaction handle obtained from
**numstore_begin**(3). If *txn* is *NULL*, the query runs in its own
automatically-managed transaction, which is committed on success or
rolled back on failure without further action from the caller.

*data* is the query's data argument, whose role depends on the query
type: it supplies source data for **INSERT** and **WRITE** queries,
and receives output data for **READ** and **REMOVE** queries. It is
unused for **CREATE**, **GET**, and **DELETE** queries and should be
*NULL* in those cases.

**numstore_fexecute**()
: Requires the caller to supply *data* already sized appropriately
  for the query - for a **READ** or **REMOVE**, *data* must already
  point to a buffer large enough to hold the requested range.

**numstore_fexecute_malloc**()
: Behaves identically to **numstore_fexecute**() for every query type
  except **READ** and **REMOVE**, for which the caller does not need
  to supply a pre-sized buffer: *data* is ignored for those two query
  types, and a buffer is instead allocated internally, sized to the
  named variable's current total length, and returned to the
  caller. For **CREATE**, **GET**, **DELETE**, **INSERT**, and
  **WRITE** queries, *data* is passed through exactly as it is to
  **numstore_fexecute**(), and the same non-NULL return value (the
  *data* pointer itself) indicates success.
:
  The pointer returned for a **READ** or **REMOVE** carries no count
  of its own - it is only guaranteed to be large enough for
  everything currently in the variable, not annotated with how many
  bytes of it are meaningful for this particular query. Callers must
  track the expected element count themselves, typically from the
  size of the range requested. The returned pointer must be released
  with the same memory facility used internally by the library (see
  the implementation for the exact allocator used), not with a
  mismatched **free**(3) if a different allocator is in use
  elsewhere in the caller's code.

# RETURN VALUE

**numstore_fexecute**() returns the number of elements read, written,
inserted, or removed for **READ**, **WRITE**, **INSERT**, and
**REMOVE** queries. For **CREATE**, **GET**, and **DELETE**, it
returns *0* on success. On failure, it returns a negative error
code.

**numstore_fexecute_malloc**() returns a non-NULL pointer on success.
For **READ** and **REMOVE**, this points to a freshly allocated
buffer containing the result (see **DESCRIPTION**); for every other
query type, this is the same *data* pointer passed in. It returns
*NULL* on failure, including if buffer allocation itself fails.

The specific error following a failure of either function can be
retrieved with **numstore_strerror**(3) or **numstore_perror**(3).

# NOTES

Both functions are declared with a **printf**(3)-style format
attribute on *query_fmt* and its variadic arguments, so callers get
compiler warnings for mismatched format specifiers under a compiler
that honors that attribute.

# EXAMPLES

Create a variable and insert formatted values:

    numstore_fexecute (db, NULL, "create %s u32", NULL, "foo");
    u32 src[5] = {10, 11, 12, 13, 14};
    numstore_fexecute (db, NULL, "insert foo 0 5", src);

Read a range into a caller-supplied buffer:

    u32 buf[5];
    numstore_fexecute (db, NULL, "read foo[0:5]", buf);

Read a range without knowing the variable's length in advance:

    u32 *data = numstore_fexecute_malloc (db, NULL, "read foo[0:]", NULL);
    if (data != NULL) {
        /* ... use data, sized to the caller's own range tracking ... */
    }

# SEE ALSO

**numstore**(1), **numstore_open**(3), **numstore_begin**(3),
**numstore_strerror**(3), **printf**(3)

# AUTHOR

Written by Theo Lincke.

# COPYRIGHT

Copyright 2026 Theo Lincke. Licensed under the Apache License,
Version 2.0. See *http://www.apache.org/licenses/LICENSE-2.0* for
details.
