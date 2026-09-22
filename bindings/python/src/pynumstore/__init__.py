"""Pythonic wrapper around the numstore C extension."""

from __future__ import annotations

from typing import Any

import numpy as np
import numpy.typing as npt

from . import _pynumstore as _ns

__all__ = ["Database", "Transaction", "Var", "to_dtype"]


def to_dtype(type_str: str) -> np.dtype[Any]:
    """Convert a numstore type string (e.g. "u32") to a numpy dtype."""
    return _ns.pyns_ns_to_np(type_str)


class Var:
    """A captured numstore variable.

    Obtained from Database.get() / Transaction.get(), or from any query that
    resolves a variable. It owns a snapshot taken when the query ran, so the
    values below do not track later inserts.

    Nothing has to be released by hand: the variable is freed as soon as the
    last reference to this object goes away.
    """

    def __init__(self, handle: Any) -> None:
        self._handle = handle

    def name(self) -> str:
        """The variable's name."""
        return _ns.pyns_var_name(self._handle)

    def length(self) -> int:
        """Number of elements stored in the variable."""
        return _ns.pyns_var_length(self._handle)

    def type(self) -> str:
        """The variable's numstore type, e.g. "u32"."""
        return _ns.pyns_var_type(self._handle)

    def tsize(self) -> int:
        """Size of a single element in bytes."""
        return _ns.pyns_var_tsize(self._handle)

    def dtype(self) -> np.dtype[Any]:
        """The variable's type as a numpy dtype."""
        return to_dtype(self.type())

    def __repr__(self) -> str:
        return (
            f"<Var {self.name()} type={self.type()!r} "
            f"length={self.length()} tsize={self.tsize()}>"
        )


class Transaction:
    """An open numstore transaction. Use via Database.begin(), not directly."""

    def __init__(self, db: "Database", handle: Any) -> None:
        self._db = db
        self._handle = handle
        self._closed = False

    def execute(
        self, query: str, data: npt.NDArray[Any] | None = None
    ) -> int | npt.NDArray[Any] | Var | None:
        return self._db._raw_execute(self._handle, query, data)

    def get(self, name: str) -> Var:
        """Look up a variable by name and capture its metadata."""
        return self._db._raw_get(self._handle, name)

    def commit(self) -> None:
        """Commit the transaction. Raises if it is already finished."""
        self._check_open()
        _ns.pyns_commit(self._db._handle, self._handle)
        self._closed = True

    def rollback(self) -> None:
        """Roll the transaction back. Raises if it is already finished."""
        self._check_open()
        _ns.pyns_rollback(self._db._handle, self._handle)
        self._closed = True

    def _check_open(self) -> None:
        if self._closed:
            raise RuntimeError("transaction was already committed or rolled back")

    def __enter__(self) -> "Transaction":
        return self

    def __exit__(self, exc_type: object, exc: object, tb: object) -> None:
        # The block is allowed to have finished the transaction by hand - only
        # a direct second commit/rollback is the caller's mistake
        if self._closed:
            return
        if exc_type is None:
            self.commit()
        else:
            self.rollback()


class Database:
    """A numstore database connection."""

    def __init__(self, path: str) -> None:
        self._handle = _ns.pyns_open(path)

    def close(self) -> None:
        if self._handle is not None:
            handle, self._handle = self._handle, None
            _ns.pyns_close(handle)

    def begin(self) -> Transaction:
        self._check_open()
        return Transaction(self, _ns.pyns_begin(self._handle))

    def _check_open(self) -> None:
        if self._handle is None:
            raise RuntimeError("database is closed")

    def execute(
        self, query: str, data: npt.NDArray[Any] | None = None
    ) -> int | npt.NDArray[Any] | Var | None:
        """Run a query. `data` is the source for insert/write, or the
        destination buffer for read/remove; given one, the element count is
        returned.

        Without `data`, the result is whatever the query produced: an array
        for a read/remove, a Var for a get, and None for a query that yields
        neither (create, delete).
        """
        self._check_open()
        return self._raw_execute(None, query, data)

    def _raw_execute(
        self, txn_handle: Any, query: str, data: npt.NDArray[Any] | None
    ) -> int | npt.NDArray[Any] | Var | None:
        result = _ns.pyns_execute(self._handle, txn_handle, query, data)

        # Anything else is the capsule a capture came back with - the only
        # place a Var is ever built.
        if result is None or isinstance(result, (int, np.ndarray)):
            return result
        return Var(result)

    def get(self, name: str) -> Var:
        """Look up a variable by name and capture its metadata."""
        self._check_open()
        return self._raw_get(None, name)

    def _raw_get(self, txn_handle: Any, name: str) -> Var:
        var = self._raw_execute(txn_handle, f"get {name}", None)
        if not isinstance(var, Var):
            raise RuntimeError(f"get {name} did not return a variable")
        return var

    def __enter__(self) -> "Database":
        return self

    def __exit__(self, exc_type: object, exc: object, tb: object) -> None:
        self.close()
