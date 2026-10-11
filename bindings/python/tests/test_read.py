from __future__ import annotations

import dataclasses

import numpy as np
import pytest

import pynumstore as ns

def test_read_allocates_an_array(db):
    db.execute("create foo u32")
    src = np.arange(5, dtype=np.uint32)
    db.write(f"insert foo 0 {src.size}", src)

    result = db.read("read foo[0:]")

    assert isinstance(result, np.ndarray)
    assert result.dtype == np.uint32
    np.testing.assert_array_equal(result, src)

def test_read_into_returns_the_count_read(db):
    db.execute("create foo u32")
    src = np.arange(3, dtype=np.uint32)
    db.write(f"insert foo 0 {src.size}", src)

    dest = np.zeros(3, dtype=np.uint32)
    assert db.read_into(f"read foo[0:] blimit {dest.nbytes}", dest) == 3

def test_read_into_is_bounded_by_the_buffer_without_blimit(db):
    db.execute("create foo u32")
    src = np.arange(10, dtype=np.uint32)
    db.write(f"insert foo 0 {src.size}", src)

    dest = np.zeros(4, dtype=np.uint32)
    assert db.read_into("read foo[0:]", dest) == dest.size
    np.testing.assert_array_equal(dest, src[:4])
