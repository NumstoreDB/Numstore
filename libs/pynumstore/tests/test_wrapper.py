"""Tests for the OOP wrapper: Database, Transaction, Variable.

Each test opens a fresh database via the tmp_path fixture, which auto-cleans
both the .db file and any sibling .wal files after the test.
"""
import os
import random
import pytest
import numpy as np
import numpy.testing as npt

pynumstore = pytest.importorskip("pynumstore")
from pynumstore import Database, Transaction, Variable


# ---------------------------------------------------------------------------
# Fixture
# ---------------------------------------------------------------------------

@pytest.fixture
def db_path(tmp_path):
    """Return a path string for a fresh database file."""
    return str(tmp_path / "test.db")


# ---------------------------------------------------------------------------
# 1. Database lifecycle
# ---------------------------------------------------------------------------

def test_db_open_close(db_path):
    db = Database(db_path)
    db.close()


def test_db_context_manager(db_path):
    with Database(db_path) as db:
        assert "test.db" in repr(db)


def test_db_close_is_idempotent(db_path):
    db = Database(db_path)
    db.close()
    db.close()  # second close should not raise


def test_db_repr_closed(db_path):
    db = Database(db_path)
    db.close()
    assert "closed" in repr(db)


def test_db_repr_open(db_path):
    with Database(db_path) as db:
        assert "closed" not in repr(db)


# ---------------------------------------------------------------------------
# 2. Variable creation and deletion
# ---------------------------------------------------------------------------

def test_create_returns_variable(db_path):
    with Database(db_path) as db:
        v = db.create("x", dtype="f32")
        assert isinstance(v, Variable)
        assert v.name == "x"


def test_create_via_var_with_flag(db_path):
    with Database(db_path) as db:
        v = db.var("x", dtype="f32", create=True)
        assert isinstance(v, Variable)


def test_create_requires_dtype_when_create_true(db_path):
    with Database(db_path) as db:
        with pytest.raises((ValueError, TypeError)):
            db.var("x", create=True)  # dtype missing


def test_delete_variable(db_path):
    with Database(db_path) as db:
        db.create("x", dtype="f32")
        db.delete("x")  # should not raise


def test_getitem_string_returns_variable(db_path):
    with Database(db_path) as db:
        db.create("x", dtype="f32")
        v = db["x"]
        assert isinstance(v, Variable)
        assert v.name == "x"


def test_getitem_non_string_raises(db_path):
    with Database(db_path) as db:
        with pytest.raises(TypeError):
            db[0]


# ---------------------------------------------------------------------------
# 3. Variable.insert and Variable.append
# ---------------------------------------------------------------------------

def test_insert_single_array(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="f32")
        v.insert(0, np.array([1.0, 2.0, 3.0], dtype=np.float32))
        assert len(v) == 1


def test_append_increments_len(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="f32")
        v.append(np.array([1.0], dtype=np.float32))
        v.append(np.array([2.0], dtype=np.float32))
        assert len(v) == 2


def test_append_returns_index(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="f32")
        idx0 = v.append(np.array([10.0], dtype=np.float32))
        idx1 = v.append(np.array([20.0], dtype=np.float32))
        assert idx0 == 0
        assert idx1 == 1


def test_insert_shifts_right(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        v.append(np.array([1], dtype=np.int32))
        v.append(np.array([3], dtype=np.int32))
        v.insert(1, np.array([2], dtype=np.int32))  # insert between
        assert len(v) == 3
        npt.assert_array_equal(v[1], np.array([2], dtype=np.int32))
        npt.assert_array_equal(v[2], np.array([3], dtype=np.int32))


def test_insert_at_zero_prepends(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        v.append(np.array([99], dtype=np.int32))
        v.insert(0, np.array([0], dtype=np.int32))
        npt.assert_array_equal(v[0], np.array([0], dtype=np.int32))
        npt.assert_array_equal(v[1], np.array([99], dtype=np.int32))


# ---------------------------------------------------------------------------
# 4. Variable.__len__ on empty variable
# ---------------------------------------------------------------------------

def test_empty_variable_len(db_path):
    with Database(db_path) as db:
        v = db.create("empty", dtype="f32")
        assert len(v) == 0


# ---------------------------------------------------------------------------
# 5. Variable.__getitem__ — read (int and slice)
# ---------------------------------------------------------------------------

def test_read_single_int_key(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        v.append(np.array([42], dtype=np.int32))
        npt.assert_array_equal(v[0], np.array([42], dtype=np.int32))


def test_read_slice_returns_ndarray(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="f32")
        for i in range(5):
            v.append(np.array([float(i)], dtype=np.float32))
        result = v[1:4]
        assert isinstance(result, np.ndarray)


def test_read_slice_values(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="f64")
        data = [10.0, 20.0, 30.0, 40.0, 50.0]
        for val in data:
            v.append(np.array([val], dtype=np.float64))
        result = v[0:5]
        assert len(result) == 5


def test_read_full_slice(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(8):
            v.append(np.array([i], dtype=np.int32))
        result = v[:]
        assert isinstance(result, np.ndarray)
        assert len(result) == 8


def test_read_empty_slice(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        v.append(np.array([1], dtype=np.int32))
        result = v[0:0]
        assert len(result) == 0


def test_read_strided_slice(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(6):
            v.append(np.array([i * 10], dtype=np.int32))
        result = v[0:6:2]  # elements 0, 2, 4
        assert len(result) == 3


def test_read_invalid_key_type_raises(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="f32")
        v.append(np.array([1.0], dtype=np.float32))
        with pytest.raises(TypeError):
            _ = v["bad"]


# ---------------------------------------------------------------------------
# 6. Variable.__setitem__ — write (overwrite)
# ---------------------------------------------------------------------------

def test_write_single_element(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        v.append(np.array([1], dtype=np.int32))
        v[0] = np.array([99], dtype=np.int32)
        npt.assert_array_equal(v[0], np.array([99], dtype=np.int32))


def test_write_does_not_change_len(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(5):
            v.append(np.array([i], dtype=np.int32))
        v[2] = np.array([999], dtype=np.int32)
        assert len(v) == 5


def test_write_slice(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(4):
            v.append(np.array([i], dtype=np.int32))
        replacement = np.array([[10], [20], [30], [40]], dtype=np.int32)
        v[0:4] = replacement
        npt.assert_array_equal(v[0], np.array([10], dtype=np.int32))
        npt.assert_array_equal(v[3], np.array([40], dtype=np.int32))


def test_write_invalid_key_type_raises(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="f32")
        v.append(np.array([1.0], dtype=np.float32))
        with pytest.raises(TypeError):
            v["bad"] = np.array([2.0], dtype=np.float32)


def test_write_preserves_neighbours(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(5):
            v.append(np.array([i], dtype=np.int32))
        v[2] = np.array([999], dtype=np.int32)
        npt.assert_array_equal(v[0], np.array([0], dtype=np.int32))
        npt.assert_array_equal(v[1], np.array([1], dtype=np.int32))
        npt.assert_array_equal(v[3], np.array([3], dtype=np.int32))
        npt.assert_array_equal(v[4], np.array([4], dtype=np.int32))
