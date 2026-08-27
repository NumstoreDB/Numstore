"""Pythonic wrapper around the numstore C extension."""

from __future__ import annotations

from typing import Any

import numpy as np
import numpy.typing as npt

from . import _pynumstore as _ns

__all__ = ["Database", "Transaction", "to_dtype"]


def to_dtype(type_str: str) -> np.dtype[Any]:
    """Convert a numstore type string (e.g. "u32") to a numpy dtype."""
    return _ns.pyns_ns_to_np(type_str)


class Transaction:
    """An open numstore transaction. Use via Database.begin(), not directly."""

    def __init__(self, db: "Database", handle: Any) -> None:
        self._db = db
        self._handle = handle
        self._closed = False

    def execute(
        self, query: str, data: npt.NDArray[Any] | None = None
    ) -> int | npt.NDArray[Any]:
        return self._db._raw_execute(self._handle, query, data)

    def commit(self) -> None:
        if self._closed:
            return
        _ns.pyns_commit(self._db._handle, self._handle)
        self._closed = True

    def rollback(self) -> None:
        if self._closed:
            return
        _ns.pyns_rollback(self._db._handle, self._handle)
        self._closed = True

    def __enter__(self) -> "Transaction":
        return self

    def __exit__(self, exc_type: object, exc: object, tb: object) -> None:
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
    ) -> int | npt.NDArray[Any]:
        """Run a query. `data` is the source for insert/write, or the
        destination buffer for read/remove. Omit it on a read/remove and an
        array is allocated and returned instead; with `data` given, the
        element count is returned.
        """
        self._check_open()
        return self._raw_execute(None, query, data)

    def _raw_execute(
        self, txn_handle: Any, query: str, data: npt.NDArray[Any] | None
    ) -> int | npt.NDArray[Any]:
        return _ns.pyns_execute(self._handle, txn_handle, query, data)

    def __enter__(self) -> "Database":
        return self

    def __exit__(self, exc_type: object, exc: object, tb: object) -> None:
        self.close()
