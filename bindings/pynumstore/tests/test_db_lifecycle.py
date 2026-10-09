from __future__ import annotations

import dataclasses

import numpy as np
import pytest

import pynumstore as ns

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
