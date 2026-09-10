---
title: NSDB_VAR_FREE
section: 3
header: Numstore Programmer's Manual
footer: numstore 1.0
date: September 2026
---

# NAME

nsdb_var_len, nsdb_var_free - inspect and release a numstore variable handle

# SYNOPSIS

**#include <numstore.h>**

*b_size*
**nsdb_var_len**(*nsdb_var_t \*var*);

*void*
**nsdb_var_free**(*nsdb_t \*db*, *nsdb_var_t \*var*);

# DESCRIPTION

*nsdb_var_t* handles are returned by variable-lookup operations such
as a **GET** query executed through **nsdb_fexecute**(3) (see
**numstore**(1)). These two functions inspect and release such a
handle.

**nsdb_var_len**()
: Returns the number of elements currently stored in the variable
  referenced by *var*. This is the variable's total byte size divided
  by the byte size of one element of its declared type, and
  corresponds to the **Nelems** field described in **numstore**(1)'s
  **GET** command.

**nsdb_var_free**()
: Releases *var* and all memory associated with it. *db* must be the
  same handle the variable was obtained from. Passing a *NULL* *var*
  is explicitly safe and is a no-op - a **GET** query issued with
  **if exists** returns a *NULL* *var* on success when the variable
  does not exist, and callers are not required to guard that case
  themselves before calling **nsdb_var_free**().

# RETURN VALUE

**nsdb_var_len**() returns the element count of *var*. It has no
failure mode of its own; *var* is assumed to be a valid, non-NULL
handle.

**nsdb_var_free**() returns nothing.

# NOTES

Every non-NULL *nsdb_var_t* handle obtained from the API should
eventually be passed to **nsdb_var_free**() to avoid leaking the
memory backing it.

# EXAMPLES

    nsdb_var_t *var;
    if (nsdb_fexecute (db, NULL, "get foo", &var) == 0) {
        b_size n = nsdb_var_len (var);
        printf ("foo has %zu elements\n", (size_t) n);
        nsdb_var_free (db, var);
    }

Handling a possibly-absent variable:

    nsdb_var_t *var;
    nsdb_fexecute (db, NULL, "get if exists foo", &var);
    if (var != NULL) {
        /* ... use var ... */
    }
    nsdb_var_free (db, var);   /* safe even if var is NULL */

# SEE ALSO

**numstore**(1), **nsdb_fexecute**(3)

# AUTHOR

Written by Theo Lincke.

# COPYRIGHT

Copyright 2026 Theo Lincke. Licensed under the Apache License,
Version 2.0. See *http://www.apache.org/licenses/LICENSE-2.0* for
details.
