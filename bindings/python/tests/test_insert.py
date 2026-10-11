from __future__ import annotations

import dataclasses

import numpy as np
import pytest

import pynumstore as ns

############# Logic
def test_create_insert_read(db):
    db.execute("create foo u32")

    src = np.arange(5, dtype=np.uint32)
    db.write(f"insert foo 0 {src.size}", src)

    dest = np.zeros(5, dtype=np.uint32)
    n = db.read_into(f"read foo[0:] blimit {dest.nbytes}", dest)

    assert n == dest.size
    np.testing.assert_array_equal(dest, src)

def test_write_returns_the_count_written(db):
    db.execute("create foo u32")
    src = np.arange(3, dtype=np.uint32)
    assert db.write(f"insert foo 0 {src.size}", src) == src.size

def test_sarray_variable_accepts_matching_2d_array(db):
    db.execute("create grid [3] u32")

    src = np.arange(6, dtype=np.uint32).reshape(2, 3)
    assert db.write("insert grid 0 2", src) == 2
    np.testing.assert_array_equal(db.read("read grid[0:]"), src)

############# Errors
def test_insert_into_nonexistent_variable_raises(db):
    with pytest.raises(RuntimeError):
        db.write("insert nope 0 1", np.zeros(1, dtype=np.uint32))
