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
    return _ns._pyns_ns_to_np(type_str)
