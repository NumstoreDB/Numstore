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


# ---------------------------------------------------------------------------
# 7. Variable.remove / __delitem__ — remove and shift left
# ---------------------------------------------------------------------------

def test_remove_single_decrements_len(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(4):
            v.append(np.array([i], dtype=np.int32))
        v.remove(0)
        assert len(v) == 3


def test_remove_single_returns_value(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        v.append(np.array([77], dtype=np.int32))
        result = v.remove(0)
        npt.assert_array_equal(result, np.array([77], dtype=np.int32))


def test_remove_shifts_left(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in [10, 20, 30]:
            v.append(np.array([i], dtype=np.int32))
        v.remove(1)  # remove 20
        npt.assert_array_equal(v[0], np.array([10], dtype=np.int32))
        npt.assert_array_equal(v[1], np.array([30], dtype=np.int32))


def test_delitem_single(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(3):
            v.append(np.array([i], dtype=np.int32))
        del v[1]
        assert len(v) == 2


def test_delitem_slice(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(6):
            v.append(np.array([i], dtype=np.int32))
        del v[2:5]
        assert len(v) == 3


def test_remove_from_tail(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="f32")
        for i in range(5):
            v.append(np.array([float(i)], dtype=np.float32))
        last = len(v) - 1
        result = v.remove(last)
        assert len(v) == 4
        npt.assert_array_equal(result, np.array([4.0], dtype=np.float32))


def test_remove_first_then_read_remaining(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in [1, 2, 3, 4]:
            v.append(np.array([i], dtype=np.int32))
        v.remove(0)
        npt.assert_array_equal(v[0], np.array([2], dtype=np.int32))
        npt.assert_array_equal(v[1], np.array([3], dtype=np.int32))
        npt.assert_array_equal(v[2], np.array([4], dtype=np.int32))


def test_remove_range_returns_ndarray(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(5):
            v.append(np.array([i], dtype=np.int32))
        result = v.remove(slice(1, 4))
        assert isinstance(result, np.ndarray)


# ---------------------------------------------------------------------------
# 8. Transaction basics
# ---------------------------------------------------------------------------

def test_begin_txn_returns_transaction(db_path):
    with Database(db_path) as db:
        txn = db.begin_txn()
        assert isinstance(txn, Transaction)
        txn.rollback()


def test_txn_context_manager_commits(db_path):
    with Database(db_path) as db:
        db.create("x", dtype="i32")
        with db.begin_txn() as txn:
            txn["x"].append(np.array([123], dtype=np.int32))
        # after commit, data should be visible
        assert len(db["x"]) == 1


def test_txn_context_manager_rollback_on_exception(db_path):
    with Database(db_path) as db:
        db.create("x", dtype="i32")
        try:
            with db.begin_txn() as txn:
                txn["x"].append(np.array([42], dtype=np.int32))
                raise RuntimeError("forced")
        except RuntimeError:
            pass
        assert len(db["x"]) == 0


def test_txn_getitem_returns_variable(db_path):
    with Database(db_path) as db:
        db.create("y", dtype="f32")
        with db.begin_txn() as txn:
            v = txn["y"]
            assert isinstance(v, Variable)
            assert v.name == "y"


def test_txn_getitem_non_string_raises(db_path):
    with Database(db_path) as db:
        with db.begin_txn() as txn:
            with pytest.raises(TypeError):
                _ = txn[0]
            txn.rollback()


def test_txn_var_method(db_path):
    with Database(db_path) as db:
        db.create("z", dtype="i64")
        with db.begin_txn() as txn:
            v = txn.var("z")
            assert isinstance(v, Variable)


def test_txn_create_variable(db_path):
    with Database(db_path) as db:
        with db.begin_txn() as txn:
            v = txn.create("newvar", dtype="f64")
            assert isinstance(v, Variable)


def test_txn_commit_and_rollback_explicit(db_path):
    with Database(db_path) as db:
        db.create("a", dtype="i32")
        txn = db.begin_txn()
        txn["a"].append(np.array([1], dtype=np.int32))
        txn.commit()
        assert len(db["a"]) == 1

        txn2 = db.begin_txn()
        txn2["a"].append(np.array([2], dtype=np.int32))
        txn2.rollback()
        assert len(db["a"]) == 1  # rollback undid the second append


def test_txn_repr(db_path):
    with Database(db_path) as db:
        with db.begin_txn() as txn:
            assert "Transaction" in repr(txn)


# ---------------------------------------------------------------------------
# 9. Multiple named variables coexist independently
# ---------------------------------------------------------------------------

def test_multiple_vars_independent_len(db_path):
    with Database(db_path) as db:
        a = db.create("a", dtype="i32")
        b = db.create("b", dtype="i32")
        a.append(np.array([1], dtype=np.int32))
        a.append(np.array([2], dtype=np.int32))
        b.append(np.array([9], dtype=np.int32))
        assert len(a) == 2
        assert len(b) == 1


def test_multiple_vars_independent_data(db_path):
    with Database(db_path) as db:
        a = db.create("a", dtype="i32")
        b = db.create("b", dtype="i32")
        a.append(np.array([111], dtype=np.int32))
        b.append(np.array([222], dtype=np.int32))
        npt.assert_array_equal(a[0], np.array([111], dtype=np.int32))
        npt.assert_array_equal(b[0], np.array([222], dtype=np.int32))


def test_delete_one_var_leaves_other(db_path):
    with Database(db_path) as db:
        a = db.create("a", dtype="i32")
        b = db.create("b", dtype="i32")
        b.append(np.array([5], dtype=np.int32))
        db.delete("a")
        npt.assert_array_equal(b[0], np.array([5], dtype=np.int32))


def test_different_dtypes(db_path):
    with Database(db_path) as db:
        fi = db.create("ints", dtype="i32")
        ff = db.create("floats", dtype="f64")
        fi.append(np.array([7], dtype=np.int32))
        ff.append(np.array([3.14], dtype=np.float64))
        npt.assert_array_equal(fi[0], np.array([7], dtype=np.int32))
        npt.assert_array_almost_equal(ff[0], np.array([3.14], dtype=np.float64))


# ---------------------------------------------------------------------------
# 10. Edge cases — boundary indices, empty operations
# ---------------------------------------------------------------------------

def test_len_after_multiple_appends(db_path):
    n = 50
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(n):
            v.append(np.array([i], dtype=np.int32))
        assert len(v) == n


def test_insert_at_end_same_as_append(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        v.append(np.array([1], dtype=np.int32))
        v.append(np.array([2], dtype=np.int32))
        # insert at end == append
        v.insert(len(v), np.array([3], dtype=np.int32))
        assert len(v) == 3
        npt.assert_array_equal(v[2], np.array([3], dtype=np.int32))


def test_write_then_read_roundtrip_i32(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        v.append(np.array([0], dtype=np.int32))
        v[0] = np.array([-2147483648], dtype=np.int32)  # INT32_MIN
        npt.assert_array_equal(v[0], np.array([-2147483648], dtype=np.int32))


def test_write_then_read_roundtrip_f64(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="f64")
        v.append(np.array([0.0], dtype=np.float64))
        val = np.float64(1.23456789012345)
        v[0] = np.array([val], dtype=np.float64)
        npt.assert_array_almost_equal(v[0], np.array([val], dtype=np.float64), decimal=12)


def test_slice_on_single_element_var(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        v.append(np.array([42], dtype=np.int32))
        result = v[0:1]
        assert len(result) == 1


def test_repr_variable(db_path):
    with Database(db_path) as db:
        v = db.create("myvar", dtype="f32")
        assert "myvar" in repr(v)


# ---------------------------------------------------------------------------
# 11. Randomized — append N, read back all, values match reference list
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("seed", [0, 1, 42, 99, 777])
def test_random_append_and_read_i32(db_path, seed):
    rng = random.Random(seed)
    n = rng.randint(10, 100)
    values = [rng.randint(-(2**30), 2**30) for _ in range(n)]

    with Database(db_path) as db:
        v = db.create("r", dtype="i32")
        for val in values:
            v.append(np.array([val], dtype=np.int32))
        assert len(v) == n
        for i, expected in enumerate(values):
            npt.assert_array_equal(v[i], np.array([expected], dtype=np.int32))


@pytest.mark.parametrize("seed", [0, 7, 13, 55, 200])
def test_random_append_and_read_f64(db_path, seed):
    rng = random.Random(seed)
    n = rng.randint(10, 80)
    values = [rng.uniform(-1e9, 1e9) for _ in range(n)]

    with Database(db_path) as db:
        v = db.create("r", dtype="f64")
        for val in values:
            v.append(np.array([val], dtype=np.float64))
        for i, expected in enumerate(values):
            npt.assert_array_almost_equal(
                v[i], np.array([expected], dtype=np.float64), decimal=6
            )


@pytest.mark.parametrize("seed", [3, 17, 88])
def test_random_write_overwrites_correctly(db_path, seed):
    rng = random.Random(seed)
    n = rng.randint(10, 50)

    with Database(db_path) as db:
        v = db.create("r", dtype="i32")
        original = [rng.randint(0, 1000) for _ in range(n)]
        for val in original:
            v.append(np.array([val], dtype=np.int32))

        # overwrite a random subset
        overwrite_indices = rng.sample(range(n), k=n // 3)
        new_vals = {}
        for idx in overwrite_indices:
            new_val = rng.randint(-9999, -1)
            v[idx] = np.array([new_val], dtype=np.int32)
            new_vals[idx] = new_val

        for i in range(n):
            expected = new_vals.get(i, original[i])
            npt.assert_array_equal(v[i], np.array([expected], dtype=np.int32))


@pytest.mark.parametrize("seed", [5, 22, 101])
def test_random_insert_then_read(db_path, seed):
    """Build a list via random inserts at random positions; compare to Python reference."""
    rng = random.Random(seed)
    n = rng.randint(5, 30)
    reference = []

    with Database(db_path) as db:
        v = db.create("r", dtype="i32")
        for _ in range(n):
            pos = rng.randint(0, len(reference))
            val = rng.randint(0, 10000)
            reference.insert(pos, val)
            v.insert(pos, np.array([val], dtype=np.int32))

        assert len(v) == len(reference)
        for i, expected in enumerate(reference):
            npt.assert_array_equal(v[i], np.array([expected], dtype=np.int32))


@pytest.mark.parametrize("seed", [6, 33, 200])
def test_random_remove_and_len(db_path, seed):
    """Append N elements, then remove one at a time from random positions."""
    rng = random.Random(seed)
    n = rng.randint(10, 40)
    reference = list(range(n))

    with Database(db_path) as db:
        v = db.create("r", dtype="i32")
        for val in reference:
            v.append(np.array([val], dtype=np.int32))

        rounds = rng.randint(3, n // 2)
        for _ in range(rounds):
            pos = rng.randint(0, len(reference) - 1)
            reference.pop(pos)
            v.remove(pos)

        assert len(v) == len(reference)
        for i, expected in enumerate(reference):
            npt.assert_array_equal(v[i], np.array([expected], dtype=np.int32))


@pytest.mark.parametrize("seed", [9, 44, 300])
def test_random_mixed_ops(db_path, seed):
    """Randomly interleave appends, writes, and removes against a Python list."""
    rng = random.Random(seed)
    reference = []

    with Database(db_path) as db:
        v = db.create("r", dtype="i32")

        for _ in range(60):
            op = rng.choice(["append", "write", "remove"])

            if op == "append" or len(reference) == 0:
                val = rng.randint(0, 9999)
                reference.append(val)
                v.append(np.array([val], dtype=np.int32))

            elif op == "write":
                idx = rng.randint(0, len(reference) - 1)
                val = rng.randint(-9999, -1)
                reference[idx] = val
                v[idx] = np.array([val], dtype=np.int32)

            elif op == "remove" and len(reference) > 1:
                idx = rng.randint(0, len(reference) - 1)
                reference.pop(idx)
                v.remove(idx)

        assert len(v) == len(reference)
        for i, expected in enumerate(reference):
            npt.assert_array_equal(v[i], np.array([expected], dtype=np.int32))


# ---------------------------------------------------------------------------
# 12. Transaction isolation and data visibility
# ---------------------------------------------------------------------------

def test_txn_writes_visible_after_commit(db_path):
    with Database(db_path) as db:
        db.create("a", dtype="i32")
        with db.begin_txn() as txn:
            txn["a"].append(np.array([1], dtype=np.int32))
            txn["a"].append(np.array([2], dtype=np.int32))
        assert len(db["a"]) == 2


def test_txn_multiple_vars_in_one_txn(db_path):
    with Database(db_path) as db:
        db.create("x", dtype="i32")
        db.create("y", dtype="f32")
        with db.begin_txn() as txn:
            txn["x"].append(np.array([10], dtype=np.int32))
            txn["y"].append(np.array([3.14], dtype=np.float32))
        assert len(db["x"]) == 1
        assert len(db["y"]) == 1


def test_txn_delete_var(db_path):
    with Database(db_path) as db:
        db.create("tmp", dtype="i32")
        with db.begin_txn() as txn:
            txn.delete("tmp")
        # no error means delete succeeded


def test_txn_create_and_populate(db_path):
    with Database(db_path) as db:
        with db.begin_txn() as txn:
            v = txn.create("fresh", dtype="i32")
            v.append(np.array([42], dtype=np.int32))
        assert len(db["fresh"]) == 1
        npt.assert_array_equal(db["fresh"][0], np.array([42], dtype=np.int32))


def test_txn_commit_then_rollback_second(db_path):
    with Database(db_path) as db:
        db.create("a", dtype="i32")
        txn = db.begin_txn()
        txn["a"].append(np.array([1], dtype=np.int32))
        txn.commit()
        assert len(db["a"]) == 1

        txn2 = db.begin_txn()
        txn2["a"].append(np.array([2], dtype=np.int32))
        txn2.rollback()
        assert len(db["a"]) == 1  # second append was discarded


def test_txn_repr_is_str(db_path):
    with Database(db_path) as db:
        with db.begin_txn() as txn:
            r = repr(txn)
            assert isinstance(r, str) and len(r) > 0


# ---------------------------------------------------------------------------
# 13. Dtype coverage — roundtrip each supported primitive
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("dtype,nptype,val", [
    ("u8",  np.uint8,   200),
    ("u16", np.uint16,  60000),
    ("u32", np.uint32,  4000000000),
    ("u64", np.uint64,  18000000000000000000),
    ("i8",  np.int8,    -100),
    ("i16", np.int16,   -30000),
    ("i32", np.int32,   -2000000000),
    ("i64", np.int64,   -9000000000000000000),
    ("f32", np.float32, 1.5),
    ("f64", np.float64, 1.23456789012345),
])
def test_dtype_roundtrip(db_path, dtype, nptype, val):
    with Database(db_path) as db:
        v = db.create("v", dtype=dtype)
        v.append(np.array([val], dtype=nptype))
        result = v[0]
        npt.assert_array_equal(result, np.array([val], dtype=nptype))


# ---------------------------------------------------------------------------
# 14. Slice semantics — start/stop/step edge cases
# ---------------------------------------------------------------------------

def test_slice_step_2(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(10):
            v.append(np.array([i], dtype=np.int32))
        result = v[0:10:2]
        assert len(result) == 5


def test_slice_negative_stop(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(5):
            v.append(np.array([i], dtype=np.int32))
        result = v[0:-1]  # all but last
        assert len(result) == 4


def test_slice_beyond_end_clamps(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(3):
            v.append(np.array([i], dtype=np.int32))
        result = v[0:1000]
        assert len(result) == 3


def test_slice_start_equals_stop(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(5):
            v.append(np.array([i], dtype=np.int32))
        result = v[2:2]
        assert len(result) == 0


def test_slice_last_element(db_path):
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(5):
            v.append(np.array([i * 10], dtype=np.int32))
        result = v[-1:]
        assert len(result) == 1


def test_slice_all_elements(db_path):
    n = 20
    with Database(db_path) as db:
        v = db.create("a", dtype="i32")
        for i in range(n):
            v.append(np.array([i], dtype=np.int32))
        result = v[:]
        assert len(result) == n
