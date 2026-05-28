"""Convert numpy dtype objects to numstore type strings.

Numstore type system:
  - Primitives:  u8/u16/u32/u64, i8/i16/i32/i64, f16/f32/f64/f128,
                 ci16/ci32/ci64/ci128, cu16/cu32/cu64/cu128,
                 cf32/cf64/cf128/cf256
  - Struct:      struct { <name> <subtype>, ... }
  - Union:       union  { <name> <subtype>, ... }
  - Strict array: [d1][d2]... <subtype>

Endianness is platform-dependent; byte order on the numpy dtype is ignored.
Numpy has no native equivalent for cf32/ci*/cu*, so this direction never
emits those primitives -- the same shapes appear as structured dtypes.
"""

from __future__ import annotations

import numpy as np


# (dtype.kind, dtype.itemsize) -> numstore primitive name.
_PRIMITIVES: dict[tuple[str, int], str] = {
    ("u", 1): "u8",   ("u", 2): "u16",  ("u", 4): "u32",  ("u", 8): "u64",
    ("i", 1): "i8",   ("i", 2): "i16",  ("i", 4): "i32",  ("i", 8): "i64",
    ("f", 2): "f16",  ("f", 4): "f32",  ("f", 8): "f64",  ("f", 16): "f128",
    ("c", 8): "cf64", ("c", 16): "cf128", ("c", 32): "cf256",
}

def numpy_to_numstore(dtype) -> str:
    """Convert a numpy dtype to a numstore type string.

    Raises ValueError if the dtype has no numstore equivalent.
    """
    if not isinstance(dtype, np.dtype):
        dtype = np.dtype(dtype)
    return _convert(dtype)


def _convert(dtype: np.dtype) -> str:
    # Strict array (subarray).
    if dtype.subdtype is not None:
        sub, shape = dtype.subdtype
        dims = "".join(f"[{d}]" for d in shape)
        return f"{dims} {_convert(sub)}"

    # Structured: distinguish struct vs union by offsets.
    if dtype.names is not None:
        assert dtype.fields is not None 
        offsets = [dtype.fields[n][1] for n in dtype.names]
        is_union = len(dtype.names) > 1 and all(o == 0 for o in offsets)
        keyword = "union" if is_union else "struct"
        parts = [f"{n} {_convert(dtype.fields[n][0])}" for n in dtype.names]
        return f"{keyword} {{ " + ", ".join(parts) + " }"

    # Primitive scalar.
    key = (dtype.kind, dtype.itemsize)
    if key not in _PRIMITIVES:
        raise ValueError(f"unsupported numpy dtype: {dtype!r}")
    return _PRIMITIVES[key]
