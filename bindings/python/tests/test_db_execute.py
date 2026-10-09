from __future__ import annotations

import dataclasses

import numpy as np
import pytest

import pynumstore as ns

def test_invalid_query_raises(db):
    with pytest.raises(RuntimeError):
        db.execute("not a real query")

    with pytest.raises(RuntimeError):
        db.read("not a real query")

    with pytest.raises(RuntimeError):
        db.write("not a real query")

    with pytest.raises(RuntimeError):
        db.get("not a real query")

def test_invalid_query_types(db):
    db.execute("create foo u32")

    db.get("get foo")
    with pytest.raises(RuntimeError):
        db.execute("get foo")

    db.write("insert foo 0 5", np.arange(5, dtype=np.uint32))
    with pytest.raises(RuntimeError):
        db.read("insert foo 0 10")

    db.write("write foo[0:5]", np.arange(5, dtype=np.uint32))
    with pytest.raises(RuntimeError):
        db.read("write foo[0:5]")

    db.read("read foo[0:10]")
    with pytest.raises(RuntimeError):
        db.write("read foo[0:10]", None, None)

    db.read("remove foo[0:10]")
    with pytest.raises(RuntimeError):
        db.write("remove foo[0:10]", None, None)

def test_get_nonexistent_variable_fails(db):
    with pytest.raises(RuntimeError):
        db.get("get foo")

