from __future__ import annotations

import dataclasses

import numpy as np
import pytest

import pynumstore as ns

############# Lifecycle tests
def test_database_context_manager_closes(db_dir):
    with ns.Database("ctx.db") as db:
        db.execute("create foo u32")
    assert db._handle is None

def test_double_close_idempodent(db_dir):
    db = ns.Database("close.db")
    db.close()
    db.close()

def test_execute_after_close_raises(db_dir):
    db = ns.Database("closed.db")
    db.close()
    with pytest.raises(RuntimeError):
        db.execute("create foo u32")

############# Invalid queries
def test_invalid_query_raises(db):
    with pytest.raises(RuntimeError):
        db.execute("not a real query")

    with pytest.raises(RuntimeError):
        db.read("not a real query")

    with pytest.raises(RuntimeError):
        db.write("not a real query", [1])

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
        db.write("read foo[0:10]", np.zeros(1, dtype=np.uint32))

    db.read("remove foo[0:10]")
    with pytest.raises(RuntimeError):
        db.write("remove foo[0:10]", np.zeros(1, dtype=np.uint32))
