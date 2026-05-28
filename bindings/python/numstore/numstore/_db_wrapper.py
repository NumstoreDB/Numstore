from __future__ import annotations

from typing import overload
import numpy as np
import numpy.typing as npt

from ._numstore import *

# ---------------------------------------------------------------------------
# Re-exported types & aliases
# ---------------------------------------------------------------------------

#: Union of index types accepted everywhere in this module.
Key = int | slice

#: A single stored element — scalar or sub-array depending on variable dtype.
Element = int | float | complex | bool | npt.NDArray


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _check_key(key: object) -> None:
    """Raise :exc:`TypeError` for anything other than ``int`` or ``slice``."""
    if not isinstance(key, (int, slice)):
        raise TypeError(f"Index must be int or slice, not {type(key).__name__!r}")


def _to_backend_key(key: Key, length: int) -> int | range:
    """Convert a Python ``Key`` to a backend-ready ``int`` or ``range``.

    Slices are resolved against *length* via :meth:`slice.indices` so the C
    layer never sees negative or open-ended indices.
    """
    _check_key(key)
    if isinstance(key, slice):
        return range(*key.indices(length))
    return key


# ---------------------------------------------------------------------------
# Mixin — shared by Database and Transaction
# ---------------------------------------------------------------------------

class _NamespaceMixin:
    """Variable-management API shared by :class:`Database` and
    :class:`Transaction`.

    Subclasses must implement the ``_db`` and ``_txn`` properties, which
    surface the raw C-extension handles.  ``_txn`` returns ``None`` when
    operating directly on the database outside any transaction.
    """

    @property
    def _db(self) -> Any:  # pragma: no cover
        raise NotImplementedError

    @property
    def _txn(self) -> Any | None:  # pragma: no cover
        raise NotImplementedError

    # -- existence -----------------------------------------------------------

    def _var_exists(self, name: str) -> bool:
        """``True`` if *name* is a known variable in this namespace.

        Uses ``var_len`` as the probe — the C layer raises ``KeyError`` for
        unknown names, which we catch here.  No separate "exists" C call is
        needed.
        """
        try:
            var_len(self._db, self._txn, name)
            return True
        except KeyError:
            return False

    def __contains__(self, name: str) -> bool:
        """Support ``"x" in db`` and ``"x" in txn``."""
        return self._var_exists(name)

    # -- retrieval -----------------------------------------------------------

    def get(self, name: str) -> "Variable":
        """Return the :class:`Variable` called *name*.

        Raises:
            KeyError: if no variable with that name exists.
        """
        if not self._var_exists(name):
            raise KeyError(f"Variable {name!r} does not exist")
        return Variable(self._db, self._txn, name)

    def __getitem__(self, name: str) -> "Variable":
        """Shorthand for :meth:`get`."""
        return self.get(name)

    def get_or_create(
        self,
        name: str,
        *,
        dtype: str | np.dtype | None = None,
    ) -> "Variable":
        """Return the variable called *name*, creating it first if absent.

        *dtype* is only required on first creation; it is silently ignored on
        subsequent calls (the existing dtype is preserved).

        Raises:
            ValueError: if the variable is absent and *dtype* is ``None``.
            TypeError:  if *dtype* is not a recognised type descriptor
                        (raised by the C layer via ``compile_type``).
        """
        if self._var_exists(name):
            return Variable(self._db, self._txn, name)
        if dtype is None:
            raise ValueError(
                f"Variable {name!r} does not exist; "
                "supply dtype= to create it automatically"
            )
        return self.create(name, dtype=dtype)

    # -- creation ------------------------------------------------------------

    def create(self, name: str, *, dtype: str | np.dtype) -> "Variable":
        """Create an empty variable and return it.

        dtype validation is performed by the C layer (``compile_type``), which
        accepts both NumPy-style names (``"float32"``) and short-hands
        (``"f32"``).

        Raises:
            KeyError:  if a variable called *name* already exists.
            TypeError: if *dtype* is not a recognised type descriptor.
        """
        # Existence check here gives a clear Python-level message; the C layer
        # would also raise KeyError but with a less friendly string.
        if self._var_exists(name):
            raise KeyError(
                f"Variable {name!r} already exists; "
                "use get_or_create() to open-or-create"
            )
        var_create(self._db, self._txn, name, dtype)
        return Variable(self._db, self._txn, name)

    def create_from(self, name: str, data: npt.NDArray) -> "Variable":
        """Create a variable whose dtype and initial contents come from *data*.

        Wraps ``var_newvar`` — a single atomic C call.

        Raises:
            KeyError: if a variable called *name* already exists.
        """
        if self._var_exists(name):
            raise KeyError(
                f"Variable {name!r} already exists"
            )
        var_newvar(self._db, self._txn, name, data)
        return Variable(self._db, self._txn, name)

    # -- deletion ------------------------------------------------------------

    def delete(self, name: str) -> None:
        """Delete the variable called *name* and all its data.

        Raises:
            KeyError: if no variable with that name exists.
        """
        if not self._var_exists(name):
            raise KeyError(f"Variable {name!r} does not exist")
        var_delete(self._db, self._txn, name)


# ---------------------------------------------------------------------------
# Database
# ---------------------------------------------------------------------------

class Database(_NamespaceMixin):
    """A numstore database file.

    Prefer :func:`numstore.open` over constructing this directly.
    Use as a context manager to ensure the file is closed reliably::

        with numstore.open("data.db") as db:
            v = db.get_or_create("signal", dtype="float32")

    Attributes:
        path: File-system path that was used to open this database.
    """

    def __init__(self, path: str) -> None:
        self.path: str = path
        self.__db: Any = db_open(path)
        self.__closed: bool = False

    # -- _NamespaceMixin contract --------------------------------------------

    @property
    def _db(self) -> Any:
        self._require_open()
        return self.__db

    @property
    def _txn(self) -> None:
        # Accessing a Database directly uses auto-commit / snapshot reads.
        return None

    # -- lifecycle -----------------------------------------------------------

    def _require_open(self) -> None:
        if self.__closed:
            raise RuntimeError(f"Operation on closed database {self.path!r}")

    @property
    def closed(self) -> bool:
        """``True`` once :meth:`close` has been called."""
        return self.__closed

    def close(self) -> None:
        """Close the database.  Idempotent — safe to call multiple times."""
        if not self.__closed:
            db_close(self.__db)
            self.__closed = True

    def begin_txn(self) -> "Transaction":
        """Begin and return a new :class:`Transaction`.

        Prefer the context-manager form so commit/rollback is automatic::

            with db.begin_txn() as txn:
                txn["x"].append(arr)   # committed on clean exit

        Raises:
            RuntimeError: if the database is closed.
        """
        self._require_open()
        return Transaction(self.__db, db_begin(self.__db))

    # -- context manager -----------------------------------------------------

    def __enter__(self) -> "Database":
        return self

    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> bool:
        self.close()
        return False

    def __repr__(self) -> str:
        status = "closed" if self.__closed else self.path
        return f"<numstore.Database {status!r}>"


# ---------------------------------------------------------------------------
# Transaction
# ---------------------------------------------------------------------------

class Transaction(_NamespaceMixin):
    """An ACID transaction on a :class:`Database`.

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

    # -- _NamespaceMixin contract --------------------------------------------

    @property
    def _db(self) -> Any:
        return self.__db

    @property
    def _txn(self) -> Any:
        self._require_active()
        return self.__txn

    # -- lifecycle -----------------------------------------------------------

    def _require_active(self) -> None:
        if self.__done:
            raise RuntimeError("Transaction has already been committed or rolled back")

    @property
    def active(self) -> bool:
        """``True`` until :meth:`commit` or :meth:`rollback` is called."""
        return not self.__done

    def commit(self) -> None:
        """Make all writes in this transaction durable.

        Raises:
            RuntimeError: if the transaction has already ended.
        """
        self._require_active()
        txn_commit(self.__txn)
        self.__done = True

    def rollback(self) -> None:
        """Discard all writes made in this transaction.

        Raises:
            RuntimeError: if the transaction has already ended.
        """
        self._require_active()
        txn_rollback(self.__txn)
        self.__done = True

    # -- context manager -----------------------------------------------------

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


# ---------------------------------------------------------------------------
# Variable
# ---------------------------------------------------------------------------

class Variable:
    """A named, typed, array-like variable stored in a :class:`Database`.

    Obtain instances from :meth:`Database.get`, :meth:`Database.create`,
    :meth:`Database.create_from`, :meth:`Database.get_or_create`, or the
    equivalent :class:`Transaction` methods.

    Indexing is restricted to ``int`` and ``slice`` — no fancy indexing::

        v[0]             # read one element
        v[2:10]          # read a range → NDArray
        v[0] = 1.0       # write one element
        v[2:10] = arr    # write a range
        del v[0]         # remove one element (shifts remainder left)
        del v[2:10]      # remove a range

    The explicit :meth:`read`, :meth:`write`, :meth:`remove`, and :meth:`pop`
    methods mirror the operators and are more readable in library code.
    """

    def __init__(
        self,
        db: Any,
        txn: Any | None,
        name: str,
    ) -> None:
        self._db: Any = db
        self._txn: Any | None = txn
        self._name: str = name

    # -- metadata ------------------------------------------------------------

    @property
    def name(self) -> str:
        """The variable's name within its database."""
        return self._name

    @property
    def dtype(self) -> np.dtype:
        """NumPy dtype corresponding to this variable's element type.

        Determined by reading one element and inspecting its dtype, so the
        variable must be non-empty.  The C layer encodes dtype in the variable
        header; a dedicated ``var_dtype`` call would be cleaner if added later.
        """
        return type_to_dtype(compile_type(
            # Round-trip through compile_type → type_to_dtype.
            # We read element [0] solely to get the type object back.
            # TODO: add a var_dtype() C call to avoid needing data for this.
            var_read(self._db, self._txn, self._name, 0)
        ))

    def __len__(self) -> int:
        """Number of elements currently stored in this variable."""
        return var_len(self._db, self._txn, self._name)

    # -- private helpers -----------------------------------------------------

    def _backend_key(self, key: Key) -> int | range:
        """Resolve *key* to a backend-ready ``int`` or ``range``."""
        return _to_backend_key(key, len(self))

    # -- read ----------------------------------------------------------------

    @overload
    def read(self, key: int)   -> Element: ...
    @overload
    def read(self, key: slice) -> npt.NDArray: ...
    def read(self, key: Key) -> Element | npt.NDArray:
        """Read elements identified by *key*.

        Args:
            key: ``int`` for a single element; ``slice`` for a contiguous range.

        Returns:
            A scalar/sub-array for ``int`` keys, an ``NDArray`` for slices.

        Raises:
            TypeError:  if *key* is not ``int`` or ``slice``.
            IndexError: if *key* is out of bounds.
        """
        return var_read(self._db, self._txn, self._name, self._backend_key(key))

    def __getitem__(self, key: Key) -> Element | npt.NDArray:
        """Alias for :meth:`read`."""
        return self.read(key)

    # -- write ---------------------------------------------------------------

    @overload
    def write(self, key: int,   value: Element)     -> None: ...
    @overload
    def write(self, key: slice, value: npt.NDArray) -> None: ...
    def write(self, key: Key, value: Element | npt.NDArray) -> None:
        """Overwrite elements at *key* with *value*.

        Args:
            key:   ``int`` or ``slice`` identifying the target positions.
            value: Data matching the variable's dtype and the length of *key*.

        Raises:
            TypeError:  if *key* is not ``int`` or ``slice``, or if *value*
                        is dtype-incompatible (raised by C layer).
            IndexError: if *key* is out of bounds.
        """
        var_write(self._db, self._txn, self._name, self._backend_key(key), value)

    def __setitem__(self, key: Key, value: Element | npt.NDArray) -> None:
        """Alias for :meth:`write`."""
        self.write(key, value)

    # -- insert / append -----------------------------------------------------

    def insert(self, offset: int, data: npt.NDArray) -> None:
        """Insert *data* at *offset*, shifting existing elements right.

        Args:
            offset: Position at which to insert (``0 <= offset <= len``).
            data:   Array of values; must be dtype-compatible with this variable.

        Raises:
            TypeError:  if *offset* is not ``int``.
            IndexError: if *offset* is out of range (raised by C layer).
            TypeError:  if *data* dtype is incompatible (raised by C layer).
        """
        if not isinstance(offset, int):
            raise TypeError(f"offset must be int, not {type(offset).__name__!r}")
        var_insert(self._db, self._txn, self._name, offset, data)

    def append(self, data: npt.NDArray) -> int:
        """Append *data* after the last element.

        Args:
            data: Array of values; must be dtype-compatible with this variable.

        Returns:
            The index of the first newly appended element.
        """
        offset = len(self)
        var_insert(self._db, self._txn, self._name, offset, data)
        return offset

    # -- remove / pop --------------------------------------------------------

    def pop(self, key: Key) -> Element | npt.NDArray:
        """Remove and *return* elements at *key*, shifting the remainder left.

        Use this when you need the removed data.  For silent removal use
        :meth:`remove` or ``del v[key]``.

        Args:
            key: ``int`` or ``slice``.

        Returns:
            Removed element(s) — scalar/sub-array for ``int``, ``NDArray``
            for slice.

        Raises:
            TypeError:  if *key* is not ``int`` or ``slice``.
            IndexError: if *key* is out of bounds.
        """
        return var_remove(self._db, self._txn, self._name, self._backend_key(key))

    def remove(self, key: Key) -> None:
        """Remove elements at *key* and discard them.

        Raises:
            TypeError:  if *key* is not ``int`` or ``slice``.
            IndexError: if *key* is out of bounds.
        """
        self.pop(key)  # return value intentionally discarded

    def __delitem__(self, key: Key) -> None:
        """Alias for :meth:`remove`; supports ``del v[0]`` and ``del v[2:10]``."""
        self.remove(key)

    # -- dunder --------------------------------------------------------------

    def __repr__(self) -> str:
        return f"<numstore.Variable {self._name!r} len={len(self)}>"


# ---------------------------------------------------------------------------
# Module-level entry point
# ---------------------------------------------------------------------------

def open(path: str) -> Database:  # noqa: A001 — intentional shadow of builtin
    """Open (or create) a numstore database at *path*.

    The recommended entry point for all database access::

        with numstore.open("data.db") as db:
            v = db.get_or_create("signal", dtype="float32")

    Args:
        path: File-system path to the database file.

    Returns:
        An open :class:`Database` instance.

    Raises:
        FileNotFoundError: if *path* is not reachable / cannot be created.
        RuntimeError:      on internal database corruption.
    """
    return Database(path)
