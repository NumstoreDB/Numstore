---
title: NSDB_BEGIN
section: 3
header: Numstore Programmer's Manual
footer: numstore 1.0
date: September 2026
---

# NAME

numstore_begin, numstore_commit, numstore_rollback - manage explicit numstore transactions

# SYNOPSIS

**#include <numstore.h>**

*ns_txn_t \**
**numstore_begin**(*numstore_t \*ns*);

*int*
**numstore_commit**(*numstore_t \*ns*, *ns_txn_t \*txn*);

*int*
**numstore_rollback**(*numstore_t \*ns*, *ns_txn_t \*txn*);

# DESCRIPTION

These functions manage explicit transactions against a **numstore**(1) database
opened with **numstore_open**(3).

**numstore_begin**() : Starts a new transaction on *ns* and returns a handle to it.
Queries executed with **numstore_fexecute**(3) or **numstore_fexecute_malloc**(3) using
this handle are grouped into the transaction and are not made durable or
visible to other transactions until it is committed.

**numstore_commit**() : Commits *txn*, making all of its changes durable and
visible to subsequent operations on *ns*. After a successful call, *txn* must
not be used again.

**numstore_rollback**() : Discards *txn* and all changes made under it, reverting
*ns* to the state it was in before the transaction began. After a successful
call, *txn* must not be used again.

Every transaction returned by **numstore_begin**() must eventually be resolved with
exactly one of **numstore_commit**() or **numstore_rollback**() - never both, and never
left unresolved.

If *txn* is passed as *NULL* to **numstore_fexecute**(3) or
**numstore_fexecute_malloc**(3) instead of an explicit transaction handle, that
call runs in its own automatically-managed transaction, which is committed on
success or rolled back on failure without the caller needing to call these
functions directly. Use explicit transactions with these three functions only
when more than one query needs to be grouped into a single atomic unit.

# RETURN VALUE

**numstore_begin**() returns a valid, non-NULL *ns_txn_t \** handle on success, or
*NULL* on failure.

**numstore_commit**() and **numstore_rollback**() return *0* on success and a non-zero
value on failure. The specific error can be retrieved with **numstore_strerror**(3)
or **numstore_perror**(3).

# EXAMPLES

Grouping two operations into one atomic transaction:

    ns_txn_t *txn = numstore_begin (db);
    if (txn == NULL) {
        /* handle failure to begin */
    }

    if (numstore_fexecute (db, txn, "create foo u32", NULL) != 0
        || numstore_fexecute (db, txn, "insert foo [1,2,3]", NULL) < 0) {
        numstore_rollback (db, txn);
    } else {
        numstore_commit (db, txn);
    }

# SEE ALSO

**numstore**(1), **numstore_open**(3), **numstore_fexecute**(3), **numstore_strerror**(3)

# AUTHOR

Written by Theo Lincke.

# COPYRIGHT

Copyright 2026 Theo Lincke. Licensed under the Apache License,
Version 2.0. See *http://www.apache.org/licenses/LICENSE-2.0* for
details.
