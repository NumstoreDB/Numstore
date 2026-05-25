from __future__ import annotations

from . import _pynumstore as _ns
from ._var import Variable
from ._txn import Transaction


class Database:
    """A numstore database.

    Open with ``Database(path)`` or the module-level ``open()``::

        with ns.open("mydb") as db:
            v = db.var("temperature", dtype="f32", create=True)
            v.append(np.float32(22.5))

            with db.begin_txn() as txn:
                txn["temperature"][0] = np.float32(99.0)
    """

    def __init__(self, path: str) -> None:
        self._path = path
        self._db = _ns.db_open(path)

    # ------------------------------------------------------------------
    # Variable access
    # ------------------------------------------------------------------

    def var(self, name: str, *, dtype=None, create: bool = False) -> Variable:
        """Return (or create) a Variable.

        Pass ``create=True`` with a *dtype* to create the variable if it
        doesn't exist yet.  *dtype* may be a numstore type string (``"f32"``)
        or a numpy dtype.
        """
        if create:
            if dtype is None:
                raise ValueError("dtype is required when create=True")
            _ns.var_create(self._db, None, name, dtype)
        return Variable(self._db, None, name)

    def __getitem__(self, name: str) -> Variable:
        """``db["varname"]`` — shorthand for ``db.var("varname")``."""
        if not isinstance(name, str):
            raise TypeError(f"variable name must be str, not {type(name).__name__}")
        return Variable(self._db, None, name)

    # ------------------------------------------------------------------
    # Variable creation / deletion
    # ------------------------------------------------------------------

    def create(self, name: str, dtype) -> Variable:
        """Create a new empty variable and return its accessor."""
        _ns.var_create(self._db, None, name, dtype)
        return Variable(self._db, None, name)

    def delete(self, name: str) -> None:
        """Drop variable *name* and all its data."""
        _ns.var_delete(self._db, None, name)

    # ------------------------------------------------------------------
    # Transactions
    # ------------------------------------------------------------------

    def begin_txn(self) -> Transaction:
        """Begin a new transaction."""
        txn = _ns.db_begin(self._db)
        return Transaction(self._db, txn)

    # ------------------------------------------------------------------
    # Lifecycle
    # ------------------------------------------------------------------

    def close(self) -> None:
        """Release all resources."""
        if self._db is not None:
            _ns.db_close(self._db)
            self._db = None

    def __enter__(self) -> Database:
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> bool:
        self.close()
        return False

    def __repr__(self) -> str:
        if self._db is not None:
            return f"<numstore.Database {self._path!r}>"
        return "<numstore.Database (closed)>"
