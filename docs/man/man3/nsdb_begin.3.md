---
title: NSDB_BEGIN
section: 3
header: Numstore Programmer's Manual
footer: numstore 1.0
date: September 2026
---

# NAME

nsdb_begin, nsdb_commit, nsdb_rollback - manage explicit numstore transactions

# SYNOPSIS

**#include <numstore.h>**

*ns_txn_t \**
**nsdb_begin**(*nsdb_t \*ns*);

*int*
**nsdb_commit**(*nsdb_t \*ns*, *ns_txn_t \*txn*);

*int*
**nsdb_rollback**(*nsdb_t \*ns*, *ns_txn_t \*txn*);

# DESCRIPTION

These functions manage explicit transactions against a **numstore**(1) database
opened with **nsdb_open**(3).

**nsdb_begin**() : Starts a new transaction on *ns* and returns a handle to it.
Queries executed with **nsdb_fexecute**(3) or **nsdb_fexecute_malloc**(3) using
this handle are grouped into the transaction and are not made durable or
visible to other transactions until it is committed.

**nsdb_commit**() : Commits *txn*, making all of its changes durable and
visible to subsequent operations on *ns*. After a successful call, *txn* must
not be used again.

**nsdb_rollback**() : Discards *txn* and all changes made under it, reverting
*ns* to the state it was in before the transaction began. After a successful
call, *txn* must not be used again.

Every transaction returned by **nsdb_begin**() must eventually be resolved with
exactly one of **nsdb_commit**() or **nsdb_rollback**() - never both, and never
left unresolved.

If *txn* is passed as *NULL* to **nsdb_fexecute**(3) or
**nsdb_fexecute_malloc**(3) instead of an explicit transaction handle, that
call runs in its own automatically-managed transaction, which is committed on
success or rolled back on failure without the caller needing to call these
functions directly. Use explicit transactions with these three functions only
when more than one query needs to be grouped into a single atomic unit.

# RETURN VALUE

**nsdb_begin**() returns a valid, non-NULL *ns_txn_t \** handle on success, or
*NULL* on failure.

**nsdb_commit**() and **nsdb_rollback**() return *0* on success and a non-zero
value on failure. The specific error can be retrieved with **nsdb_strerror**(3)
or **nsdb_perror**(3).

# EXAMPLES

Grouping two operations into one atomic transaction:

    ns_txn_t *txn = nsdb_begin (db);
    if (txn == NULL) {
        /* handle failure to begin */
    }

    if (nsdb_fexecute (db, txn, "create foo u32", NULL) != 0
        || nsdb_fexecute (db, txn, "insert foo [1,2,3]", NULL) < 0) {
        nsdb_rollback (db, txn);
    } else {
        nsdb_commit (db, txn);
    }

# SEE ALSO

**numstore**(1), **nsdb_open**(3), **nsdb_fexecute**(3), **nsdb_strerror**(3)

# AUTHOR

Written by Theo Lincke.

# COPYRIGHT

Copyright 2026 Theo Lincke. Licensed under the Apache License,
Version 2.0. See *http://www.apache.org/licenses/LICENSE-2.0* for
details.
