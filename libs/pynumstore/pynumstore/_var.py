from __future__ import annotations

import numpy as np
from . import _pynumstore as _ns


class Variable:
    """A named, typed array stream within a numstore database.

    Obtained via ``Database.var()``, ``Database["name"]``,
    ``Transaction.var()``, or ``Transaction["name"]``.
    """

    def __init__(self, db: object, txn: object, name: str) -> None:
        self._db = db
        self._txn = txn
        self._name = name

    # ------------------------------------------------------------------
    # Length
    # ------------------------------------------------------------------

    def __len__(self) -> int:
        return _ns.var_len(self._db, self._txn, self._name)

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _to_range(self, key: slice) -> range:
        return range(*key.indices(len(self)))

    # ------------------------------------------------------------------
    # Read
    # ------------------------------------------------------------------

    def __getitem__(self, key):
        if isinstance(key, int):
            return _ns.var_read(self._db, self._txn, self._name, key)
        if isinstance(key, slice):
            return _ns.var_read(self._db, self._txn, self._name, self._to_range(key))
        raise TypeError(f"key must be int or slice, not {type(key).__name__}")

    # ------------------------------------------------------------------
    # Write (overwrite existing entries)
    # ------------------------------------------------------------------

    def __setitem__(self, key, value) -> None:
        if isinstance(key, int):
            _ns.var_write(self._db, self._txn, self._name, key, value)
        elif isinstance(key, slice):
            _ns.var_write(self._db, self._txn, self._name, self._to_range(key), value)
        else:
            raise TypeError(f"key must be int or slice, not {type(key).__name__}")

    # ------------------------------------------------------------------
    # Insert / append (creates new entries, shifts right)
    # ------------------------------------------------------------------

    def insert(self, ofst: int, data: np.ndarray) -> None:
        """Insert *data* at *ofst*, shifting existing elements right."""
        _ns.var_insert(self._db, self._txn, self._name, ofst, data)

    def append(self, data: np.ndarray) -> int:
        """Append *data* at the end; returns the new element's index."""
        ofst = len(self)
        _ns.var_insert(self._db, self._txn, self._name, ofst, data)
        return ofst

    # ------------------------------------------------------------------
    # Remove (removes entries, shifts left)
    # ------------------------------------------------------------------

    def remove(self, key: int | slice):
        """Remove and return element(s), shifting remaining elements left."""
        if isinstance(key, slice):
            return _ns.var_remove(self._db, self._txn, self._name, self._to_range(key))
        return _ns.var_remove(self._db, self._txn, self._name, key)

    def __delitem__(self, key) -> None:
        """Remove element(s) without returning them."""
        self.remove(key)

    # ------------------------------------------------------------------
    # Repr
    # ------------------------------------------------------------------

    @property
    def name(self) -> str:
        return self._name

    def __repr__(self) -> str:
        return f"<numstore.Variable {self._name!r}>"
