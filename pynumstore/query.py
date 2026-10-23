"""One-off query methods shared by Database and Transaction."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any
from . import _pynumstore as _ns
from .utils import handle_to_var

if TYPE_CHECKING:
    import numpy.typing as npt

    from .database import Database
    from .transaction import Transaction
    from .var import Var


class Queries:
    def _target(self) -> tuple[Database, Transaction | None]:
        raise NotImplementedError

    def execute(self, query: str) -> None:
        db, txn = self._target()
        with db.prepare(query) as plan:
            plan.execute(txn)

    def read(self, query: str) -> npt.NDArray[Any]:
        db, txn = self._target()
        with db.prepare(query) as plan:
            return plan.read(txn)

    def read_into(self, query: str, out: npt.NDArray[Any]) -> int:
        db, txn = self._target()
        with db.prepare(query) as plan:
            return plan.read_into(out, txn)

    def write(self, query: str, data: npt.ArrayLike) -> int:
        db, txn = self._target()
        with db.prepare(query) as plan:
            return plan.write(data, txn)

    def get(self, query: str) -> Var:
        db, txn = self._target()
        with db.prepare(query) as plan:
            return plan.var(txn)

    def get_all(self) -> List[Var]:
        db, txn = self._target()

        db._require_open()

        _db = db._handle
        _tx = None
        if txn is not None:
            if txn._db is not db:
                raise ValueError("transaction belongs to a different database")
            _tx = txn._handle

        vars = _ns._pyns_get_all_vars(_db, _tx)
        return [handle_to_var(v) for v in vars]
