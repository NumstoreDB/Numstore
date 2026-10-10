"""Type conversion and array validation helpers."""

from __future__ import annotations

from math import prod
from typing import TYPE_CHECKING, Any

import numpy as np
import numpy.typing as npt

from . import _pynumstore as _ns
from .var import Var

if TYPE_CHECKING:
    from ._pynumstore import nsplan, nstxn


def to_dtype(type_str: str) -> np.dtype[Any]:
    return _ns._pyns_ns_to_np(type_str)


def capture_var(plan: nsplan, txn: nstxn | None) -> Var:
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


def layout(var: Var) -> tuple[np.dtype[Any], tuple[int, ...]]:
    shape, dtype = _ns._pyns_ns_to_np_flatten(var.type, 0)
    dims = shape[1:]
    if dtype.itemsize * prod(dims) != var.tsize:
        raise RuntimeError(
            f"numpy and numstore disagree on the size of {var.type!r}: "
            f"{dtype.itemsize * prod(dims)} vs {var.tsize} bytes"
        )
    return dtype, dims


def check_dims(shape: tuple[int, ...], dims: tuple[int, ...], what: str) -> None:
    if dims and tuple(shape[-len(dims) :]) != dims:
        expected = ", ".join(["n", *map(str, dims)])
        raise ValueError(f"{what} has shape {shape}, expected ({expected})")


def as_source(
    data: npt.ArrayLike, dtype: np.dtype[Any], dims: tuple[int, ...]
) -> npt.NDArray[Any]:
    if isinstance(data, np.ma.MaskedArray):
        raise TypeError("masked arrays aren't supported; use .filled() first")

    if isinstance(data, np.ndarray):
        if data.dtype != dtype:
            if not np.can_cast(data.dtype, dtype, "same_kind"):
                raise TypeError(f"cannot write {data.dtype} data to a {dtype} variable")
            data = data.astype(dtype)
        arr = data
    else:
        arr = np.asarray(data, dtype=dtype)

    check_dims(arr.shape, dims, "data")
    return np.require(arr, dtype=dtype, requirements=["C", "A"])
