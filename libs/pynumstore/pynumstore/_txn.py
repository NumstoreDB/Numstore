from __future__ import annotations

from . import _pynumstore as _ns
from ._var import Variable


class Transaction:
    """Atomic unit of work against a numstore database.

    Obtain via ``Database.begin_txn()`` or use as a context manager::

        with db.begin_txn() as txn:
            txn["temperature"][0:10] = arr

    Commits on clean exit; rolls back on any exception.
    """

    def __init__(self, db: object, txn: object) -> None:
        self._db = db
        self._txn = txn

    # ------------------------------------------------------------------
    # Variable access
    # ------------------------------------------------------------------

    def var(self, name: str) -> Variable:
        """Return a Variable accessor for *name* bound to this transaction."""
        return Variable(self._db, self._txn, name)

    def __getitem__(self, name: str) -> Variable:
        """``txn["varname"]`` — shorthand for ``txn.var("varname")``."""
        if not isinstance(name, str):
            raise TypeError(f"variable name must be str, not {type(name).__name__}")
        return Variable(self._db, self._txn, name)

    # ------------------------------------------------------------------
    # Variable creation / deletion
    # ------------------------------------------------------------------

    def create(self, name: str, dtype) -> Variable:
        """Create a new empty variable and return its accessor."""
        _ns.var_create(self._db, self._txn, name, dtype)
        return Variable(self._db, self._txn, name)

    def delete(self, name: str) -> None:
        """Drop variable *name* and all its data."""
        _ns.var_delete(self._db, self._txn, name)

    # ------------------------------------------------------------------
    # Lifecycle
    # ------------------------------------------------------------------

    def commit(self) -> None:
        """Commit all pending mutations."""
        _ns.txn_commit(self._txn)

    def rollback(self) -> None:
        """Discard all pending mutations."""
        _ns.txn_rollback(self._txn)

    # ------------------------------------------------------------------
    # Context manager
    # ------------------------------------------------------------------

    def __enter__(self) -> Transaction:
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> bool:
        if exc_type is None:
            self.commit()
        else:
            self.rollback()
        return False

    def __repr__(self) -> str:
        return "<numstore.Transaction>"
