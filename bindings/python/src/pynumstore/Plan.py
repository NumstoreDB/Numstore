from __future__ import annotations

from dataclasses import dataclass
from math import prod
from typing import Any

import numpy as np
import numpy.typing as npt
from . import _pynumstore as _ns

class Plan:
    def __init__(self, db: Database, query: str) -> None:
        self._db = db
        self._query = query

        # Create a handle
        handle = _ns._pyns_plan_create(db._require_open(), query)
        self._handle: nsplan | None = handle

    @property
    def query(self) -> str:
        return self._query

    @property
    def closed(self) -> bool:
        return self._handle is None or self._db.closed

    def execute(self, txn: Transaction | None = None) -> int:
        return _ns._pyns_plan_execute(*self._unwrap_self_tx(txn))

    def get(self, txn: Transaction | None = None) -> Var:
        return _capture_var(*self._unwrap_self_tx(txn))

    def read(self, txn: Transaction | None = None) -> npt.NDArray[Any]:
        # First, get the variable that this query represents
        plan, txh = self._unwrap_self_tx(txn)
        var = capture_var(plan, txh)
        dtype, dims = layout(var)

        # Then read in the bytes
        raw = _ns._pyns_plan_malloc(plan, txh)

        # Verify - pynumstore.c already does this
        # but nice to do it in python too
        if var.tsize == 0 or len(raw) % var.tsize != 0:
            raise AssertionError(
                f"read returned {len(raw)} bytes, not a multiple of {var.tsize}"
            )

        arr = np.frombuffer(raw, dtype).reshape(-1, *dims)

        return arr if arr.flags.writeable else arr.copy()

    def read_into(self, out: npt.NDArray[Any], txn: Transaction | None = None) -> int:
        plan, txh = self._unwrap_self_tx(txn)
        dtype, dims = _layout(_capture_var(plan, txh))

        if not isinstance(out, np.ndarray):
            raise TypeError(f"out must be a numpy array, not {type(out).__name__}")
        if out.dtype != dtype:
            raise TypeError(f"out has dtype {out.dtype}, expected {dtype}")
        _check_dims(out.shape, dims, "out")
        if not (out.flags.c_contiguous and out.flags.writeable):
            raise ValueError("out must be C-contiguous and writable")

        return _ns._pyns_plan_read(plan, txh, out)

    def write(self, data: npt.ArrayLike, txn: Transaction | None = None) -> int:
        plan, txh = self._unwrap_self_tx(txn)
        dtype, dims = _layout(_capture_var(plan, txh))
        return _ns._pyns_plan_write(plan, txh, _as_source(data, dtype, dims))

    def close(self) -> None:
        if self._handle is not None:
            handle, self._handle = self._handle, None
            _ns._pyns_plan_close(handle)

    def _unwrap_self_tx(self, txn: Transaction | None) -> tuple[nsplan, nstxn | None]:
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

