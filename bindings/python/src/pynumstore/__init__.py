"""Pythonic interface to numstore.

    import numpy as np
    import pynumstore as ns

    with ns.Database("data.ns") as db:
        db.execute("create foo u32")
        db.write("insert foo 0 3", [1, 2, 3])
        arr = db.read("read foo[0:3]")          # numpy array of uint32

        with db.begin() as tx:                   # commit on success, rollback on error
            tx.write("write foo[0:1]", [9])
            tx.execute("create bar f64")

        reader = db.prepare("read foo[0:]")      # compile once, run many times
        latest = reader.read()

Queries outside a transaction run in their own automatic transaction.
"""

from __future__ import annotations

import os
from dataclasses import dataclass
from math import prod
from typing import TYPE_CHECKING, Any

import numpy as np
import numpy.typing as npt

from . import _pynumstore as _ns

if TYPE_CHECKING:
    from types import TracebackType

    from ._pynumstore import nsdb, nsplan, nstxn

__all__ = ["Database", "Plan", "Transaction", "Var", "to_dtype"]


def to_dtype(type_str: str) -> np.dtype[Any]:
    """Convert a numstore type string (e.g. "u32") to the equivalent numpy dtype."""
    return _ns._pyns_ns_to_np(type_str)


# ------------------------------------------------------------------ Var


@dataclass(frozen=True)
class Var:
    """A numstore variable's metadata, captured when it was looked up.

    A plain value: it holds no database resources and doesn't track later
    changes. `shape` and `dtype` describe the whole variable as a numpy array,
    so `np.empty(var.shape, var.dtype)` can hold all of it.

    Attributes:
        name:   the variable's name
        type:   element type as a numstore type string, e.g. "u32"
        length: number of elements
        tsize:  size of one element in bytes
    """

    name: str
    type: str
    length: int
    tsize: int

    @property
    def shape(self) -> tuple[int, ...]:
        """`(length, *dims)`: leading strict-array dims of the type move into the shape."""
        return _ns._pyns_ns_to_np_flatten(self.type, self.length)[0]

    @property
    def dtype(self) -> np.dtype[Any]:
        """dtype of one scalar in `shape` (the type minus its leading strict-array dims)."""
        return _ns._pyns_ns_to_np_flatten(self.type, 0)[1]

    def __len__(self) -> int:
        return self.length


# ------------------------------------------------------------------ Helpers


def _capture_var(plan: nsplan, txn: nstxn | None) -> Var:
    """Snapshot the plan's variable into a Var and free the C handle right away."""
    handle = _ns._pyns_plan_get_var(plan, txn)
    try:
        return Var(
            name=_ns._pyns_var_name(handle),
            type=_ns._pyns_var_type(handle),
            length=_ns._pyns_var_length(handle),
            tsize=_ns._pyns_var_tsize(handle),
        )
    finally:
        _ns._pyns_var_free(handle)


def _layout(var: Var) -> tuple[np.dtype[Any], tuple[int, ...]]:
    """Scalar dtype and per-element dims, checked against numstore's element size."""
    shape, dtype = _ns._pyns_ns_to_np_flatten(var.type, 0)
    dims = shape[1:]
    if dtype.itemsize * prod(dims) != var.tsize:
        raise RuntimeError(
            f"numpy and numstore disagree on the size of {var.type!r}: "
            f"{dtype.itemsize * prod(dims)} vs {var.tsize} bytes"
        )
    return dtype, dims


def _check_dims(shape: tuple[int, ...], dims: tuple[int, ...], what: str) -> None:
    if dims and tuple(shape[-len(dims) :]) != dims:
        expected = ", ".join(["n", *map(str, dims)])
        raise ValueError(f"{what} has shape {shape}, expected ({expected})")


def _as_source(
    data: npt.ArrayLike, dtype: np.dtype[Any], dims: tuple[int, ...]
) -> npt.NDArray[Any]:
    """Turn `data` into a C-contiguous, native-byte-order array of `dtype`."""
    if isinstance(data, np.ma.MaskedArray):
        # The mask lives outside the data buffer, so it would be silently dropped
        raise TypeError("masked arrays aren't supported; use .filled() first")

    if isinstance(data, np.ndarray):
        # Arrays: allow float64 -> float32, byte swaps, etc., but not float -> int
        if data.dtype != dtype:
            if not np.can_cast(data.dtype, dtype, "same_kind"):
                raise TypeError(f"cannot write {data.dtype} data to a {dtype} variable")
            data = data.astype(dtype)
        arr = data
    else:
        # Lists and scalars: convert directly (out-of-range ints raise)
        arr = np.asarray(data, dtype=dtype)

    _check_dims(arr.shape, dims, "data")
    return np.require(arr, dtype=dtype, requirements=["C", "A"])


# ------------------------------------------------------------------ Plan


class Plan:
    """A compiled query that can be run many times. Create with Database.prepare().

    Every method runs the query once. Pass `txn` to run inside a transaction;
    otherwise the query runs in its own automatic transaction.

    The plan is freed when garbage collected, or explicitly with close() or a
    `with` block.
    """

    def __init__(self, db: Database, query: str) -> None:
        self._db = db
        self._query = query
        self._handle: nsplan | None = _ns._pyns_plan_create(db._require_open(), query)

    @property
    def query(self) -> str:
        return self._query

    @property
    def closed(self) -> bool:
        return self._handle is None or self._db.closed

    def execute(self, txn: Transaction | None = None) -> int:
        """Run a query that takes no data (create, delete, remove, ...)."""
        return _ns._pyns_plan_execute(*self._args(txn))

    def var(self, txn: Transaction | None = None) -> Var:
        """Look up the variable this query refers to, without running the query."""
        return _capture_var(*self._args(txn))

    def read(self, txn: Transaction | None = None) -> npt.NDArray[Any]:
        """Run a read/remove query and return the result as a new array.

        The array has shape `(n, *dims)` and the variable's scalar dtype - see
        Var.shape and Var.dtype.
        """
        plan, txh = self._args(txn)
        var = _capture_var(plan, txh)
        dtype, dims = _layout(var)

        raw = _ns._pyns_plan_malloc(plan, txh)
        if var.tsize == 0 or len(raw) % var.tsize != 0:
            raise RuntimeError(
                f"read returned {len(raw)} bytes, not a multiple of {var.tsize}"
            )

        arr = np.frombuffer(raw, dtype).reshape(-1, *dims)

        # Arrays over bytes are read-only; over a bytearray they're writable
        # with no copy needed
        return arr if arr.flags.writeable else arr.copy()

    def read_into(self, out: npt.NDArray[Any], txn: Transaction | None = None) -> int:
        """Run a read/remove query into `out`, a preallocated array.

        `out` must be writable and C-contiguous, with the variable's scalar
        dtype and trailing dims (np.empty(var.shape, var.dtype) qualifies).
        Returns the count numstore reports.
        """
        plan, txh = self._args(txn)
        dtype, dims = _layout(_capture_var(plan, txh))

        # Validate rather than convert: a converted copy would receive the
        # data, and `out` would silently stay untouched
        if not isinstance(out, np.ndarray):
            raise TypeError(f"out must be a numpy array, not {type(out).__name__}")
        if out.dtype != dtype:
            raise TypeError(f"out has dtype {out.dtype}, expected {dtype}")
        _check_dims(out.shape, dims, "out")
        if not (out.flags.c_contiguous and out.flags.writeable):
            raise ValueError("out must be C-contiguous and writable")

        return _ns._pyns_plan_read(plan, txh, out)

    def write(self, data: npt.ArrayLike, txn: Transaction | None = None) -> int:
        """Run an insert/write query with `data` as the source.

        Lists and scalars are converted to the variable's dtype. Arrays are
        cast if that keeps their kind (float64 -> float32 is fine, float ->
        int is an error). Returns the count numstore reports.
        """
        plan, txh = self._args(txn)
        dtype, dims = _layout(_capture_var(plan, txh))
        return _ns._pyns_plan_write(plan, txh, _as_source(data, dtype, dims))

    def close(self) -> None:
        """Free the plan. Safe to call twice, or after the database was closed."""
        if self._handle is not None:
            handle, self._handle = self._handle, None
            _ns._pyns_plan_close(handle)

    def _args(self, txn: Transaction | None) -> tuple[nsplan, nstxn | None]:
        if self._handle is None:
            raise RuntimeError("plan is closed")
        self._db._require_open()
        if txn is None:
            return self._handle, None
        if txn._db is not self._db:
            raise ValueError("transaction belongs to a different database")
        return self._handle, txn._require_open()

    def __enter__(self) -> Plan:
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc: BaseException | None,
        tb: TracebackType | None,
    ) -> None:
        self.close()

    def __repr__(self) -> str:
        state = " closed" if self.closed else ""
        return f"<Plan {self._query!r}{state}>"


# ------------------------------------------------------------------ One-off queries


class _Queries:
    """Query methods shared by Database and Transaction.

    Each call compiles the query, runs it once and frees it. To run the same
    query many times, compile it once with Database.prepare().
    """

    def _target(self) -> tuple[Database, Transaction | None]:
        raise NotImplementedError

    def execute(self, query: str) -> int:
        """Run a query that takes no data (create, delete, remove, ...)."""
        db, txn = self._target()
        with db.prepare(query) as plan:
            return plan.execute(txn)

    def read(self, query: str) -> npt.NDArray[Any]:
        """Run a read/remove query and return the result as a new array."""
        db, txn = self._target()
        with db.prepare(query) as plan:
            return plan.read(txn)

    def read_into(self, query: str, out: npt.NDArray[Any]) -> int:
        """Run a read/remove query into a preallocated array. See Plan.read_into."""
        db, txn = self._target()
        with db.prepare(query) as plan:
            return plan.read_into(out, txn)

    def write(self, query: str, data: npt.ArrayLike) -> int:
        """Run an insert/write query with `data` as the source. See Plan.write."""
        db, txn = self._target()
        with db.prepare(query) as plan:
            return plan.write(data, txn)

    def get(self, query: str) -> Var:
        """Look up a variable by name."""
        db, txn = self._target()
        with db.prepare(query) as plan:
            return plan.var(txn)


# ------------------------------------------------------------------ Transaction


class Transaction(_Queries):
    """An open transaction. Create with Database.begin().

    As a context manager it commits when the block succeeds and rolls back
    when it raises. Either way it's finished afterwards, even if the commit
    or rollback itself fails.
    """

    def __init__(self, db: Database, handle: nstxn) -> None:
        self._db = db
        self._handle: nstxn | None = handle

    @property
    def closed(self) -> bool:
        return self._handle is None

    def commit(self) -> None:
        handle, self._handle = self._require_open(), None
        _ns._pyns_commit(self._db._require_open(), handle)

    def rollback(self) -> None:
        handle, self._handle = self._require_open(), None
        _ns._pyns_rollback(self._db._require_open(), handle)

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


# ------------------------------------------------------------------ Database


class Database(_Queries):
    """A numstore database. Use as a context manager, or call close()."""

    def __init__(self, path: str | os.PathLike[str]) -> None:
        self._path = os.fspath(path)
        self._handle: nsdb | None = _ns._pyns_open(self._path)

    @property
    def path(self) -> str:
        return self._path

    @property
    def closed(self) -> bool:
        return self._handle is None

    def close(self) -> None:
        """Close the database. Safe to call twice. Plans made from it stop working."""
        if self._handle is not None:
            handle, self._handle = self._handle, None
            _ns._pyns_close(handle)

    def begin(self) -> Transaction:
        """Begin a transaction. Best used as `with db.begin() as tx:`."""
        return Transaction(self, _ns._pyns_begin(self._require_open()))

    def prepare(self, query: str) -> Plan:
        """Compile a query once so it can be run many times."""
        return Plan(self, query)

    def _require_open(self) -> nsdb:
        if self._handle is None:
            raise RuntimeError("database is closed")
        return self._handle

    def _target(self) -> tuple[Database, Transaction | None]:
        return self, None

    def __enter__(self) -> Database:
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc: BaseException | None,
        tb: TracebackType | None,
    ) -> None:
        self.close()

    def __repr__(self) -> str:
        state = " closed" if self.closed else ""
        return f"<Database {self._path!r}{state}>"
