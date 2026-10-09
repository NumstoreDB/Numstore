from math import prod
from typing import Any

import numpy as np
import numpy.typing as npt

from pynumstore.Variable import Variable
from . import _pynumstore as _ns

def validate_numpy_array_for_ns_dtype(
    data: npt.ArrayLike,   # Input data
    dtype: np.dtype[Any],  # The data type of the variable
    dims: tuple[int, ...]  # The variable dims
) -> npt.NDArray[Any]:
    """
    Converts [data] to an array         
    """

    if isinstance(data, np.ma.MaskedArray):
        raise TypeError("masked arrays aren't supported; use .filled() first")

    if isinstance(data, np.ndarray):
        if data.dtype != dtype:
            if not np.can_cast(data.dtype, dtype, "same_kind"):
                raise TypeError(f"cannot write {data.dtype} data to a {dtype} variable")
            data = data.astype(dtype)
        arr = data
    else:
        # Lists and scalars: convert directly (out-of-range ints raise)
        arr = np.asarray(data, dtype=dtype)

    # 
    if dims and tuple(arr.shape[-len(dims) :]) != dims:
        expected = ", ".join(["n", *map(str, dims)])
        raise ValueError(f"data has shape {arr.shape}, expected ({expected})")

    # Return a valid numpy array 
    #   C row major ordering 
    #   A Aligned
    return np.require(arr, dtype=dtype, requirements=["C", "A"])

def capture_var(plan, txn) -> Variable:
    """
    Gets a variable once, populates all fields 
    then free's the variable
    """
    handle = _ns._pyns_plan_get_var(plan, txn)

    try:
        # Core database properties
        name=_ns._pyns_var_name(handle)
        type=_ns._pyns_var_type(handle)
        length=_ns._pyns_var_length(handle)
        tsize=_ns._pyns_var_tsize(handle)

        # Get it's flattened data type
        shape, dtype = _ns._pyns_ns_to_np_flatten(type, length)
        assert shape[0] == length
        tshape = shape[1:]

        return Variable(
                name = name,
                type = type,
                length = length,
                tsize = tsize,
                shape = shape,
                dtype = dtype,
                tshape = tshape
        )
    finally:
        _ns._pyns_var_free(handle)

def layout(var: Variable) -> tuple[np.dtype[Any], tuple[int, ...]]:
    """
    Normalizes a variable's type to a numpy dtype and 
    a shape.  

    [10][20] struct { a u32, b f16 } 
    will return:
    [("a", uint32), ("b", float16)], (10, 20)
    """

    # Get the shape and dtype of a length 0 array
    shape, dtype = _ns._pyns_ns_to_np_flatten(var.type, 0)

    # shape = [0, ....] - take the rest to the right
    dims = shape[1:]

    # [10][20] struct { a u32, b u32 }
    # 10 * 20 * sizeof(struct { ... }) == var.tsize
    if dtype.itemsize * prod(dims) != var.tsize:
        raise AssertionError(
            f"numpy and numstore disagree on the size of {var.type!r}: "
            f"{dtype.itemsize * prod(dims)} vs {var.tsize} bytes"
        )

    return dtype, dims
