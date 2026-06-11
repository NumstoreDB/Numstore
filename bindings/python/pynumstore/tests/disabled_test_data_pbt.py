import pytest
import numpy as np
from hypothesis import given, assume
from hypothesis import strategies as st
from hypothesis.strategies import composite
from hypothesis.stateful import RuleBasedStateMachine, rule, invariant

import pynumstore as ns

# ---------------------------------------------------------------------------
# Strategies
# ---------------------------------------------------------------------------

MAX_SIZE = 32

F32_ARRAYS = st.lists(
    st.floats(allow_nan=False, allow_infinity=False, width=32),
    min_size=1,
    max_size=MAX_SIZE,
).map(lambda l: np.array(l, dtype=np.float32))


@composite
def array_and_index(draw):
    """An array and a valid insertion index into it (0..len inclusive)."""
    arr = draw(F32_ARRAYS)
    idx = draw(st.integers(min_value=0, max_value=len(arr)))
    return arr, idx


@composite
def array_and_slice(draw):
    """An array and a non-empty slice (start, end) within its bounds."""
    arr = draw(F32_ARRAYS)
    start = draw(st.integers(min_value=0, max_value=len(arr) - 1))
    end   = draw(st.integers(min_value=start + 1, max_value=len(arr)))
    return arr, start, end


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def delete_if_exists(db, name):
    try:
        db.delete(name)
    except RuntimeError:
        pass


def fresh_var(db, name, typ="f32"):
    """Delete-if-exists then create, returning an empty variable."""
    delete_if_exists(db, name)
    db.get_or_create(name, typ)
    return db.get(name)


def setup_var(db, name, initial):
    """Fresh variable pre-loaded with initial data."""
    var = fresh_var(db, name)
    var.append(initial)
    return var


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

_session_db = None   # populated by conftest autouse fixture


class TestDataProperties:

    @pytest.fixture(autouse=True)
    def _db(self, db):
        self.db = db

    # -----------------------------------------------------------------------
    # Append
    # -----------------------------------------------------------------------

    @given(arr=F32_ARRAYS)
    def test_append_roundtrip(self, arr):
        var = fresh_var(self.db, "t_append")
        var.append(arr)
        np.testing.assert_array_equal(var[0:], arr)

    @given(a=F32_ARRAYS, b=F32_ARRAYS)
    def test_two_appends_concatenate(self, a, b):
        var = fresh_var(self.db, "t_append2")
        var.append(a)
        var.append(b)
        np.testing.assert_array_equal(var[0:], np.concatenate([a, b]))

    @given(initial=F32_ARRAYS, extra=F32_ARRAYS)
    def test_append_grows_length(self, initial, extra):
        var = setup_var(self.db, "t_applen", initial)
        old_len = len(var[0:])
        var.append(extra)
        assert len(var[0:]) == old_len + len(extra)

    # -----------------------------------------------------------------------
    # Insert
    # -----------------------------------------------------------------------

    @given(data=array_and_index(), extra=F32_ARRAYS)
    def test_insert_position(self, data, extra):
        initial, idx = data
        var = setup_var(self.db, "t_insert", initial)
        var.insert(idx, extra)
        expected = np.concatenate([initial[:idx], extra, initial[idx:]])
        np.testing.assert_array_equal(var[0:], expected)

    @given(initial=F32_ARRAYS, extra=F32_ARRAYS)
    def test_insert_at_zero_is_prepend(self, initial, extra):
        var = setup_var(self.db, "t_prepend", initial)
        var.insert(0, extra)
        np.testing.assert_array_equal(var[0:], np.concatenate([extra, initial]))

    @given(initial=F32_ARRAYS, extra=F32_ARRAYS)
    def test_insert_at_end_is_append(self, initial, extra):
        var = setup_var(self.db, "t_ins_end", initial)
        var.insert(len(initial), extra)
        np.testing.assert_array_equal(var[0:], np.concatenate([initial, extra]))

    @given(data=array_and_index(), extra=F32_ARRAYS)
    def test_insert_grows_length(self, data, extra):
        initial, idx = data
        var = setup_var(self.db, "t_inslen", initial)
        old_len = len(var[0:])
        var.insert(idx, extra)
        assert len(var[0:]) == old_len + len(extra)

    # -----------------------------------------------------------------------
    # Overwrite
    # -----------------------------------------------------------------------

    @given(data=array_and_slice(), patch=F32_ARRAYS)
    def test_overwrite_only_affects_slice(self, data, patch):
        initial, start, end = data
        actual_patch = np.array(patch[:end - start], dtype=np.float32)
        assume(len(actual_patch) == end - start)

        var = setup_var(self.db, "t_write", initial)
        var[start:end] = actual_patch

        expected = initial.copy()
        expected[start:end] = actual_patch
        np.testing.assert_array_equal(var[0:], expected)

    @given(data=array_and_slice(), patch=F32_ARRAYS)
    def test_overwrite_does_not_change_length(self, data, patch):
        initial, start, end = data
        actual_patch = np.array(patch[:end - start], dtype=np.float32)
        assume(len(actual_patch) == end - start)

        var = setup_var(self.db, "t_writelen", initial)
        var[start:end] = actual_patch
        assert len(var[0:]) == len(initial)

    # -----------------------------------------------------------------------
    # Delete
    # -----------------------------------------------------------------------

    @given(data=array_and_slice())
    def test_delete_slice_correct_elements_remain(self, data):
        initial, start, end = data
        var = setup_var(self.db, "t_del", initial)
        del var[start:end]
        expected = np.concatenate([initial[:start], initial[end:]])
        np.testing.assert_array_equal(var[0:], expected)

    @given(data=array_and_slice())
    def test_delete_shrinks_length(self, data):
        initial, start, end = data
        var = setup_var(self.db, "t_dellen", initial)
        old_len = len(var[0:])
        del var[start:end]
        assert len(var[0:]) == old_len - (end - start)

    @given(initial=F32_ARRAYS)
    def test_delete_all_leaves_empty(self, initial):
        var = setup_var(self.db, "t_delall", initial)
        del var[0:]
        assert len(var[0:]) == 0

    # -----------------------------------------------------------------------
    # Rollback
    # -----------------------------------------------------------------------

    def _committed_var(self, name, initial):
        """Create and populate a variable inside a committed transaction."""
        delete_if_exists(self.db, name)
        with self.db.begin_txn() as txn:
            txn.get_or_create(name, "f32")
            txn.get(name).append(initial)
        return self.db.get(name)[0:].copy()

    @given(initial=F32_ARRAYS, extra=F32_ARRAYS)
    def test_rollback_append(self, initial, extra):
        snapshot = self._committed_var("t_rb_app", initial)
        with pytest.raises(RuntimeError):
            with self.db.begin_txn() as txn:
                txn.get("t_rb_app").append(extra)
                raise RuntimeError("forced rollback")
        np.testing.assert_array_equal(self.db.get("t_rb_app")[0:], snapshot)

    @given(data=array_and_index(), extra=F32_ARRAYS)
    def test_rollback_insert(self, data, extra):
        initial, idx = data
        snapshot = self._committed_var("t_rb_ins", initial)
        with pytest.raises(RuntimeError):
            with self.db.begin_txn() as txn:
                txn.get("t_rb_ins").insert(idx, extra)
                raise RuntimeError("forced rollback")
        np.testing.assert_array_equal(self.db.get("t_rb_ins")[0:], snapshot)

    @given(data=array_and_slice(), patch=F32_ARRAYS)
    def test_rollback_write(self, data, patch):
        initial, start, end = data
        actual_patch = np.array(patch[:end - start], dtype=np.float32)
        assume(len(actual_patch) == end - start)

        snapshot = self._committed_var("t_rb_wri", initial)
        with pytest.raises(RuntimeError):
            with self.db.begin_txn() as txn:
                txn.get("t_rb_wri")[start:end] = actual_patch
                raise RuntimeError("forced rollback")
        np.testing.assert_array_equal(self.db.get("t_rb_wri")[0:], snapshot)

    @given(data=array_and_slice())
    def test_rollback_delete(self, data):
        initial, start, end = data
        snapshot = self._committed_var("t_rb_del", initial)
        with pytest.raises(RuntimeError):
            with self.db.begin_txn() as txn:
                var = txn.get("t_rb_del")
                del var[start:end]
                raise RuntimeError("forced rollback")
        np.testing.assert_array_equal(self.db.get("t_rb_del")[0:], snapshot)

