"""One-off query methods shared by Database and Transaction."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    import numpy.typing as npt

    from .database import Database
    from .transaction import Transaction
    from .var import Var


class Queries:
    def _target(self) -> tuple[Database, Transaction | None]:
        raise NotImplementedError

    def execute(self, query: str) -> int:
        db, txn = self._target()
        with db.prepare(query) as plan:
            return plan.execute(txn)

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
