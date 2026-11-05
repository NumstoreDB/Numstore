"""Var: a snapshot of a numstore variable's metadata."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

import numpy as np

from . import _pynumstore as _ns


@dataclass(frozen=True)
class Var:
    name: str
    type: str
    length: int
    tsize: int

    @property
    def shape(self) -> tuple[int, ...]:
        return _ns._pyns_ns_to_np_flatten(self.type, self.length)[0]

    @property
    def dtype(self) -> np.dtype[Any]:
        return _ns._pyns_ns_to_np_flatten(self.type, 0)[1]

    def __len__(self) -> int:
        return self.length
