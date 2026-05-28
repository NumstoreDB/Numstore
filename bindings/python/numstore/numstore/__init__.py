from contextlib import contextmanager

from .numstore_numpy import numpy_to_numstore
from ._numstore import *
from ._numstore import var_create as _c_var_create
from ._db_wrapper import Database, Transaction, Variable


@contextmanager
def open_db(path):
    """Context manager that opens a database and closes it on exit."""
    db = db_open(path)
    try:
        yield db
    finally:
        db_close(db)


@contextmanager
def transaction(db):
    """Context manager for a transaction.

    Commits on clean exit; rolls back on any exception.
    """
    txn = db_begin(db)
    try:
        yield txn
        txn_commit(txn)
    except BaseException:
        txn_rollback(txn)
        raise


def var_create(db, txn, var, dtype):
    """Create a variable named *var* with the given *dtype*.

    *dtype* may be a numstore type string (e.g. ``"f32"``) or a numpy dtype.
    """
    if not isinstance(dtype, str):
        dtype = numpy_to_numstore(dtype)
    _c_var_create(db, txn, var, dtype)


def var_newvar(db, txn, var, data):
    """Create variable *var* with dtype inferred from *data* and populate it.

    Equivalent to ``var_create`` followed by ``var_insert`` at offset 0.
    """
    var_create(db, txn, var, numpy_to_numstore(data.dtype))
    var_insert(db, txn, var, 0, data)


def open(path: str) -> Database:
    """Open (or create) a numstore database; returns a ``Database`` context manager."""
    return Database(path)


__all__ = [
    # OOP wrappers
    "Database",
    "Transaction",
    "Variable",
    "open",
    # low-level helpers (re-exported from C extension)
    "open_db",
    "transaction",
    "var_create",
    "var_newvar",
    "numpy_to_numstore",
]
