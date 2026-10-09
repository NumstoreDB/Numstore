
class QueryExecutor:
    """
    A thing that can execute queries
    """
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
            return plan.get(txn)

