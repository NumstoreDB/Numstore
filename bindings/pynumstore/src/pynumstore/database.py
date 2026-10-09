import os

from . import _pynumstore as _ns

from pynumstore.Plan import Plan
from pynumstore.Transaction import Transaction
from pynumstore.QueryExecutor import QueryExecutor

class Database(QueryExecutor):

    def __init__(self, path: str | os.PathLike[str]) -> None:
        # Normalize the path
        self._path = os.fspath(path)

        # Open the database
        db_handle = _ns._pyns_open(self._path)
        self._handle: nsdb | None = handle

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
        tx_handle = _ns._pyns_begin(self._require_open())
        return Transaction(self, tx_handle)

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
