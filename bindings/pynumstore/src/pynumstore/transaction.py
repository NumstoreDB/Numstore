class Transaction(QueryExecutor):

    def __init__(self, db: Database, handle: nstxn) -> None:
        self._db = db
        self._handle: nstxn | None = handle

    @property
    def closed(self) -> bool:
        return self._handle is None

    def commit(self) -> None:
        tx_handle = self._require_open()
        db_handle = self._db._require_open()
        _ns._pyns_commit(db_handle, tx_handle)
        self._handle = None

    def rollback(self) -> None:
        tx_handle = self._require_open()
        db_handle = self._db._require_open()
        _ns._pyns_rollback(db_handle, tx_handle)
        self._handle = None

    def _require_open(self) -> nstxn:
        if self._handle is None:
            raise RuntimeError("transaction was already committed or rolled back")
        return self._handle

    def _target(self) -> tuple[Database, Transaction | None]:
        self._require_open()
        return self._db, self

    def __enter__(self) -> Transaction:
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc: BaseException | None,
        tb: TracebackType | None,
    ) -> None:
        # The block may have finished the transaction itself
        if self._handle is None:
            return
        if exc_type is None:
            self.commit()
        else:
            self.rollback()

    def __repr__(self) -> str:
        return f"<Transaction {'closed' if self.closed else 'open'}>"


