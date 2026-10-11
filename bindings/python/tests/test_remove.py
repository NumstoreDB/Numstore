from __future__ import annotations

import dataclasses

import numpy as np
import pytest

import pynumstore as ns

def test_read_of_a_remove_returns_the_removed_data(db):
    db.execute("create foo u32")
    src = np.arange(5, dtype=np.uint32)
    db.write(f"insert foo 0 {src.size}", src)

    removed = db.read("remove foo[0:2]")
    np.testing.assert_array_equal(removed, src[0:2])

    remaining = db.read("read foo[0:]")
    np.testing.assert_array_equal(remaining, src[2:])

