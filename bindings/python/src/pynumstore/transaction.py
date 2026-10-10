"""Transaction: an open numstore transaction."""

from __future__ import annotations

from typing import TYPE_CHECKING

from . import _pynumstore as _ns
from .query import Queries

if TYPE_CHECKING:
    from types import TracebackType

    from ._pynumstore import nstxn
    from .database import Database


class Transaction(Queries):
    def __init__(self, db: Database, handle: nstxn) -> None:
        self._db = db
        self._handle: nstxn | None = handle

    @property
    def closed(self) -> bool:
        return self._handle is None

    def commit(self) -> None:
        handle, self._handle = self._require_open(), None
        _ns._pyns_commit(self._db._require_open(), handle)

    def rollback(self) -> None:
        handle, self._handle = self._require_open(), None
        _ns._pyns_rollback(self._db._require_open(), handle)

    def _require_open(self) -> nstxn:
        if self._handle is None:
            raise RuntimeError("transaction was already committed or rolled back")
        return self._handle

    def _target(self) -> tuple[Database, Transaction | None]:
        self._require_open()
        return self._db, self

    def __enter__(self) -> Transaction:
        return self

    def __exit__(self,exc_type,exc,tb) -> None:
        if self._handle is None:
            return

        if exc_type is None:
            self.commit()
        else:
            self.rollback()

    def __repr__(self) -> str:
        return f"<Transaction {'closed' if self.closed else 'open'}>"
