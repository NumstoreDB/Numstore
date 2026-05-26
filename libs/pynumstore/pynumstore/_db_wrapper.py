from __future__ import annotations

from . import _pynumstore as _ns

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

    def delete(self, name: str) -> None:
        """Drop variable *name* and all its data."""
        _ns.var_delete(self._db, None, name)

    def begin_txn(self) -> Transaction:
        """Begin a new transaction."""
        return Transaction(self._db, _ns.db_begin(self._db))

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

    def var(self, name: str, *, dtype=None, create: bool = False) -> Variable:
        """Return (or create) a Variable bound to this transaction.

        Pass ``create=True`` with a *dtype* to create the variable if it
        doesn't exist yet.  *dtype* may be a numstore type string (``"f32"``)
        or a numpy dtype.
        """
        if create:
            if dtype is None:
                raise ValueError("dtype is required when create=True")
            _ns.var_create(self._db, self._txn, name, dtype)
        return Variable(self._db, self._txn, name)

    def __getitem__(self, name: str) -> Variable:
        """``txn["varname"]`` — shorthand for ``txn.var("varname")``."""
        if not isinstance(name, str):
            raise TypeError(f"variable name must be str, not {type(name).__name__}")
        return Variable(self._db, self._txn, name)

    def delete(self, name: str) -> None:
        """Drop variable *name* and all its data."""
        _ns.var_delete(self._db, self._txn, name)

    def commit(self) -> None:
        """Commit all pending mutations."""
        _ns.txn_commit(self._txn)

    def rollback(self) -> None:
        """Discard all pending mutations."""
        _ns.txn_rollback(self._txn)

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


class Variable:
    """A named, typed array stream within a numstore database.

    Obtained via ``Database.var()``, ``Database["name"]``,
    ``Transaction.var()``, or ``Transaction["name"]``.
    """

    def __init__(self, db: object, txn: object, name: str) -> None:
        self._db = db
        self._txn = txn
        self._name = name

    @property
    def name(self) -> str:
        return self._name

    def __len__(self) -> int:
        return _ns.var_len(self._db, self._txn, self._name)

    def _to_range(self, key: slice) -> range:
        return range(*key.indices(len(self)))

    def __getitem__(self, key):
        if isinstance(key, int):
            return _ns.var_read(self._db, self._txn, self._name, key)
        if isinstance(key, slice):
            return _ns.var_read(self._db, self._txn, self._name, self._to_range(key))
        raise TypeError(f"key must be int or slice, not {type(key).__name__}")

    def __setitem__(self, key, value) -> None:
        if isinstance(key, int):
            _ns.var_write(self._db, self._txn, self._name, key, value)
        elif isinstance(key, slice):
            _ns.var_write(self._db, self._txn, self._name, self._to_range(key), value)
        else:
            raise TypeError(f"key must be int or slice, not {type(key).__name__}")

    def insert(self, ofst: int, data: np.ndarray) -> None:
        """Insert *data* at *ofst*, shifting existing elements right."""
        _ns.var_insert(self._db, self._txn, self._name, ofst, data)

    def append(self, data: np.ndarray) -> int:
        """Append *data* at the end; returns the new element's index."""
        ofst = len(self)
        _ns.var_insert(self._db, self._txn, self._name, ofst, data)
        return ofst

    def remove(self, key: int | slice):
        """Remove and return element(s), shifting remaining elements left."""
        if isinstance(key, slice):
            return _ns.var_remove(self._db, self._txn, self._name, self._to_range(key))
        return _ns.var_remove(self._db, self._txn, self._name, key)

    def __delitem__(self, key) -> None:
        """Remove element(s) without returning them."""
        self.remove(key)

    def __repr__(self) -> str:
        return f"<numstore.Variable {self._name!r}>"
