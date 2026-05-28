from __future__ import annotations
from typing import Any

import numpy.typing as npt
import numpy as np

from ._numstore import *

#############################################
##### Primary API

def open(path) -> Database:
    """Context manager that opens a database and closes it on exit."""
    return Database(path)

#############################################
##### Objects

class _NumstoreMixin:
    """Variable-management API shared by :class:`Database` and
    :class:`Transaction`.

    Subclasses must implement the ``_db`` and ``_txn`` properties, which
    surface the raw C-extension handles.  ``_txn`` returns ``None`` when
    operating directly on the database outside any transaction.
    """

    @property
    def _db(self) -> Any:
        raise NotImplementedError

    @property
    def _txn(self) -> Any | None:
        raise NotImplementedError

    def _var_exists(self, name: str) -> bool:
        return var_exists(self._db, self._txn, name)

    def get(self, name: str) -> "Variable":
        if not self._var_exists(name):
            raise RuntimeError(f"Variable {name!r} does not exist")
        return Variable(self._db, self._txn, name)

    def get_or_create(self, name: str, dtype: str | None):
        if self._var_exists(name):
            return Variable(self._db, self._txn, name)
        if dtype is None:
            raise RuntimeError(
                    "Variable: {name} does not exist -"
                    " therefore you must pass a data type to get_or_create"
            )
        return self.create(name, dtype=dtype)

    def create(self, name: str, dtype: str):
        if self._var_exists(name):
            raise RuntimeError(
                f"Variable {name!r} already exists; "
                "use get_or_create() to open-or-create"
            )
        var_create(self._db, self._txn, name, dtype)
        return Variable(self._db, self._txn, name)

    def delete(self, name: str) -> None:
        if not self._var_exists(name):
            raise RuntimeError(f"Variable {name!r} does not exist")
        var_delete(self._db, self._txn, name)

    ############# Operator Overloading

    def __contains__(self, name: str) -> bool:
        return self._var_exists(name)

    def __getitem__(self, name: str) -> "Variable":
        return self.get(name)


class Database(_NumstoreMixin):
    """A numstore database file.

    Attributes:
        path: File-system path that was used to open this database.
        numstore is single file - so this is just a file
    """

    def __init__(self, path: str) -> None:
        self.path: str = path
        self.__db: Any = db_open(path)
        self.__closed: bool = False

    ############# Primary API

    @property
    def closed(self) -> bool:
        return self.__closed

    def close(self) -> None:
        if not self.__closed:
            db_close(self.__db)
            self.__closed = True

    def begin_txn(self) -> "Transaction":
        self._require_open()
        return Transaction(self.__db, db_begin(self.__db))

    ############# _NumstoreMixin

    @property
    def _db(self) -> Any:
        self._require_open()
        return self.__db

    @property
    def _txn(self) -> None:
        return None

    ############# Utils

    def _require_open(self) -> None:
        if self.__closed:
            raise RuntimeError(f"Operation on closed database {self.path!r}")

    ############# Operator Overloading

    def __enter__(self) -> "Database":
        return self

    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> bool:
        self.close()
        return False

    def __repr__(self) -> str:
        status = "closed" if self.__closed else self.path
        return f"<numstore.Database {status!r}>"


class Transaction(_NumstoreMixin):
    """A transaction reference on a :class:`Database`.

    Obtain via :meth:`Database.begin_txn`.  As a context manager it commits
    on clean exit and rolls back on any exception::

        with db.begin_txn() as txn:
            txn.create("scratch", dtype="int32")
            txn["scratch"].append(arr)
        # auto-committed here

    Manual :meth:`commit` / :meth:`rollback` are also supported, after which
    the transaction must not be used again.
    """

    def __init__(self, db: Any, txn: Any) -> None:
        self.__db: Any = db
        self.__txn: Any = txn
        self.__done: bool = False

    ############# Primary API

    @property
    def active(self) -> bool:
        return not self.__done

    def commit(self) -> None:
        self._require_active()
        txn_commit(self.__txn)
        self.__done = True

    def rollback(self) -> None:
        self._require_active()
        txn_rollback(self.__txn)
        self.__done = True

    ############# Utils

    def _require_active(self) -> None:
        if self.__done:
            raise RuntimeError("Transaction has already been committed or rolled back")

    ############# _NumstoreMixin

    @property
    def _db(self) -> Any:
        return self.__db

    @property
    def _txn(self) -> Any:
        self._require_active()
        return self.__txn

    ############# Operator Overloads

    def __enter__(self) -> "Transaction":
        return self

    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> bool:
        if not self.__done:
            if exc_type is None:
                self.commit()
            else:
                self.rollback()
        return False

    def __repr__(self) -> str:
        return f"<numstore.Transaction ({'active' if self.active else 'done'})>"

class Variable:
    """A named, typed, array-like variable stored in a :class:`Database`.

    Obtain instances from :meth:`Database.get`, :meth:`Database.create`,
    :meth:`Database.create_from`, :meth:`Database.get_or_create`, or the
    equivalent :class:`Transaction` methods.

    Indexing is restricted to ``slice`` - no fancy indexing::

        v[2:10]          # read a range -> NDArray
        v[2:10] = arr    # write a range
        del v[2:10]      # remove a range

    The explicit :meth:`read`, :meth:`write`, :meth:`remove`, and :meth:`pop`
    methods mirror the operators and are more readable in library code.
    """

    def __init__(self, db: Any, txn: Any | None, name: str) -> None:
        self._db: Any = db
        self._txn: Any | None = txn
        self._name: str = name

    ############# Primary Api

    @property
    def name(self) -> str:
        return self._name

    def read(self, key: range) -> npt.NDArray:
        return var_read(self._db, self._txn, self._name, key)

    def write(self, key: range, value: npt.NDArray) -> None:
        var_write(self._db, self._txn, self._name, key, value)

    def insert(self, offset: int, data: npt.NDArray) -> None:
        var_insert(self._db, self._txn, self._name, offset, data)

    def append(self, data: npt.NDArray) -> None:
        offset = len(self)
        var_insert(self._db, self._txn, self._name, offset, data)

    def remove(self, key: range) -> npt.NDArray:
        return var_remove(self._db, self._txn, self._name, key)

    ############# Operator Overloads

    def __len__(self) -> int:
        return var_len(self._db, self._txn, self._name)

    def __getitem__(self, key: range) -> npt.NDArray:
        return self.read(key)

    def __setitem__(self, key: range, value: npt.NDArray) -> None:
        self.write(key, value)

    def __delitem__(self, key: range) -> None:
        self.remove(key)

    def __repr__(self) -> str:
        return f"<numstore.Variable {self._name!r} len={len(self)}>"

#############################################
##### np_to_ns / ns_to_np

_PRIMITIVES: dict[tuple[str, int], str] = {
    ("u", 1): "u8",   ("u", 2): "u16",  ("u", 4): "u32",  ("u", 8): "u64",
    ("i", 1): "i8",   ("i", 2): "i16",  ("i", 4): "i32",  ("i", 8): "i64",
    ("f", 2): "f16",  ("f", 4): "f32",  ("f", 8): "f64",  ("f", 16): "f128",
    ("c", 8): "cf64", ("c", 16): "cf128", ("c", 32): "cf256",
}

def nstype_to_dtype(type: str) -> np.dtype:
    return ns_to_np(type)

def dtype_to_nstype(dtype) -> str:
    if not isinstance(dtype, np.dtype):
        dtype = np.dtype(dtype)
    return _convert(dtype)

def _convert(dtype: np.dtype) -> str:
    if dtype.subdtype is not None:
        sub, shape = dtype.subdtype
        dims = "".join(f"[{d}]" for d in shape)
        return f"{dims} {_convert(sub)}"

    if dtype.names is not None:
        assert dtype.fields is not None 
        offsets = [dtype.fields[n][1] for n in dtype.names]
        is_union = len(dtype.names) > 1 and all(o == 0 for o in offsets)
        keyword = "union" if is_union else "struct"
        parts = [f"{n} {_convert(dtype.fields[n][0])}" for n in dtype.names]
        return f"{keyword} {{ " + ", ".join(parts) + " }"

    key = (dtype.kind, dtype.itemsize)
    if key not in _PRIMITIVES:
        raise ValueError(f"unsupported numpy dtype: {dtype!r}")

    return _PRIMITIVES[key]
