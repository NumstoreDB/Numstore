"""Database: an open numstore database."""

from __future__ import annotations

import os
from typing import TYPE_CHECKING

from . import _pynumstore as _ns
from .plan import Plan
from .query import Queries
from .transaction import Transaction

if TYPE_CHECKING:
    from types import TracebackType

    from ._pynumstore import nsdb


class Database(Queries):

    def __init__(self, path: str | os.PathLike[str]) -> None:
        self._path = os.fspath(path)
        self._handle: nsdb | None = _ns._pyns_open(self._path)

    @property
    def path(self) -> str:
        return self._path

    @property
    def closed(self) -> bool:
        return self._handle is None

    def close(self) -> None:
        if self._handle is not None:
            handle, self._handle = self._handle, None
            _ns._pyns_close(handle)

    def begin(self) -> Transaction:
        return Transaction(self, _ns._pyns_begin(self._require_open()))

    def prepare(self, query: str) -> Plan:
        return Plan(self, query)

    def _require_open(self) -> nsdb:
        if self._handle is None:
            raise RuntimeError("database is closed")
        return self._handle

    def _target(self) -> tuple[Database, Transaction | None]:
        return self, None

    def __enter__(self) -> Database:
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()

    def __repr__(self) -> str:
        state = " closed" if self.closed else ""
        return f"<Database {self._path!r}{state}>"
