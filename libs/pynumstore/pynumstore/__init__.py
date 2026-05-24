from contextlib import contextmanager

from .numstore_numpy import numpy_to_numstore
from ._pynumstore import *
from ._pynumstore import var_create as _c_var_create


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
