from __future__ import annotations
from contextlib import contextmanager
from typing import Generator, overload
import numpy as np
import numpy.typing as npt


# ---------------------------------------------------------------------------
# Opaque handle types
# ---------------------------------------------------------------------------

class Database:
    """Opaque handle to an open numstore database."""
    __slots__ = ("_handle",)

class Transaction:
    """
    Opaque handle to an active transaction.
    Invalidated after txn_commit or txn_rollback — do not reuse.
    """
    __slots__ = ("_handle",)


# ---------------------------------------------------------------------------
# Type aliases
# ---------------------------------------------------------------------------

# None means no explicit transaction (auto-commit behaviour).
Txn = Transaction | None

# One element's worth of data — a primitive scalar, or an NDArray matching
# the element shape for multi-dimensional variable types.
Element = int | float | complex | bool | npt.NDArray


# ---------------------------------------------------------------------------
# Type conversion
# ---------------------------------------------------------------------------

def ns_to_np(s: str) -> np.dtype:
    """Convert a numstore type string (e.g. ``"f32"``, ``"i64"``) to a numpy dtype."""
    ...


# ---------------------------------------------------------------------------
# Database lifecycle
# ---------------------------------------------------------------------------

def db_open(path: str) -> Database:
    """Open (or create) a numstore database at *path*."""
    ...

def db_close(db: Database) -> None:
    """Close the database. All transactions must be committed or rolled back first."""
    ...

@contextmanager
def open_db(path: str) -> Generator[Database, None, None]:
    """
    Context manager that opens a database and closes it on exit.

    Example::

        with open_db("mydb") as db:
            ...
    """
    db = db_open(path)
    try:
        yield db
    finally:
        db_close(db)


# ---------------------------------------------------------------------------
# Transaction lifecycle
# ---------------------------------------------------------------------------

def db_begin(db: Database) -> Transaction:
    """Begin a new transaction."""
    ...

def txn_commit(txn: Transaction) -> None:
    """Commit *txn*. The handle is invalid after this call."""
    ...

def txn_rollback(txn: Transaction) -> None:
    """Roll back *txn*. The handle is invalid after this call."""
    ...

@contextmanager
def transaction(db: Database) -> Generator[Transaction, None, None]:
    """
    Context manager for a transaction.
    Commits on clean exit; rolls back on any exception.

    Example::

        with transaction(db) as txn:
            var_write(db, txn, "temperature", 0, np.float32(22.5))
    """
    txn = db_begin(db)
    try:
        yield txn
        txn_commit(txn)
    except BaseException:
        txn_rollback(txn)
        raise


# ---------------------------------------------------------------------------
# Variable management
# ---------------------------------------------------------------------------

def var_create(db: Database, txn: Txn, var: str, dtype: str | np.dtype) -> None:
    """
    Create an empty variable named *var* with the given *dtype*.
    *dtype* may be a numstore type string (e.g. ``"f32"``) or a numpy dtype.
    """
    ...

def var_newvar(db: Database, txn: Txn, var: str, data: npt.NDArray) -> None:
    """
    Create variable *var* with dtype inferred from *data* and populate it.
    Equivalent to ``var_create`` followed by ``var_insert`` at offset 0.
    """
    ...

def var_delete(db: Database, txn: Txn, var: str) -> None:
    """Delete variable *var* and all its data."""
    ...

def var_len(db: Database, txn: Txn, var: str) -> int:
    """Return the number of elements stored in *var*."""
    ...


# ---------------------------------------------------------------------------
# Element-level read / write / insert / remove
# ---------------------------------------------------------------------------

@overload
def var_read(db: Database, txn: Txn, var: str, key: int)   -> Element: ...
@overload
def var_read(db: Database, txn: Txn, var: str, key: range) -> npt.NDArray: ...
def var_read(db: Database, txn: Txn, var: str, key: int | range) -> Element | npt.NDArray:
    """
    Read elements from *var*.

    - ``key: int``   → returns one element (scalar or sub-array depending on dtype).
    - ``key: range`` → returns an NDArray of ``len(key)`` elements.
    """
    ...

def var_insert(db: Database, txn: Txn, var: str, ofst: int, data: npt.NDArray) -> None:
    """
    Insert *data* into *var* at element offset *ofst*, shifting existing
    elements right. *data* must have a dtype compatible with *var*.
    """
    ...

@overload
def var_write(db: Database, txn: Txn, var: str, key: int,   data: Element)     -> None: ...
@overload
def var_write(db: Database, txn: Txn, var: str, key: range, data: npt.NDArray) -> None: ...
def var_write(db: Database, txn: Txn, var: str, key: int | range, data: Element | npt.NDArray) -> None:
    """
    Overwrite elements in *var* at position *key*.

    - ``key: int``   → *data* is one element (scalar or sub-array).
    - ``key: range`` → *data* is an NDArray whose length matches *key*,
                       with a dtype compatible with *var*.
    """
    ...

@overload
def var_remove(db: Database, txn: Txn, var: str, key: int)   -> Element: ...
@overload
def var_remove(db: Database, txn: Txn, var: str, key: range) -> npt.NDArray: ...
def var_remove(db: Database, txn: Txn, var: str, key: int | range) -> Element | npt.NDArray:
    """
    Remove and return elements from *var* at position *key*, shifting
    remaining elements left.

    - ``key: int``   → removes and returns one element.
    - ``key: range`` → removes and returns an NDArray of ``len(key)`` elements.
    """
    ...
