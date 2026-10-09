from __future__ import annotations

from dataclasses import dataclass
from math import prod
from typing import Any

import numpy as np
import numpy.typing as npt
from . import _pynumstore as _ns

@dataclass(frozen=True)
class Variable:
    name: str
    type: str
    length: int
    tsize: int
    shape: tuple[int,...]
    dtype: np.dtype[Any]
    tshape: tuple[int,...]

    def __len__(self) -> int:
        return self.length
