from __future__ import annotations

import dataclasses

import numpy as np
import pytest

import pynumstore as ns

def test_get_returns_a_var(db):
    db.execute("create foo u32")

    var = db.get("get foo")
    assert isinstance(var, ns.Var)
    assert var.type == "u32"

def test_var_composite_type(db):
    db.execute("create foo struct { a u32, b f64 }")

    var = db.get("get foo")
    assert var.type == "struct { a u32, b f64 }"
    assert var.tsize == 12
    assert var.length == 0

def test_var_after_database_close_raises(db_dir):
    db = ns.Database("varclose.db")
    db.execute("create foo u32")
    db.close()

    with pytest.raises(RuntimeError):
        db.get("get foo")

def test_get_nonexistent_variable_fails(db):
    with pytest.raises(RuntimeError):
        db.get("get foo")
