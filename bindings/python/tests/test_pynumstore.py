from __future__ import annotations

import numpy as np
import pytest

import pynumstore as ns


def test_to_dtype():
    assert ns.to_dtype("u32") == np.dtype(np.uint32)
    assert ns.to_dtype("f64") == np.dtype(np.float64)
    assert ns.to_dtype("i8") == np.dtype(np.int8)
    assert ns.to_dtype("u8") == np.dtype(np.uint8)


def test_to_dtype_invalid_type_raises():
    with pytest.raises(ValueError):
        ns.to_dtype("not_a_real_type")


def test_database_context_manager_closes(tmp_path):
    with ns.Database(str(tmp_path / "ctx.db")) as db:
        db.execute("create foo u32")
    assert db._handle is None


def test_double_close_raises_cleanly(tmp_path):
    db = ns.Database(str(tmp_path / "close.db"))
    db.close()
    db.close()


def test_execute_after_close_raises(tmp_path):
    db = ns.Database(str(tmp_path / "closed.db"))
    db.close()
    with pytest.raises(RuntimeError):
        db.execute("create foo u32")


def test_execute_invalid_query_raises(db):
    with pytest.raises(RuntimeError):
        db.execute("not a real query")


def test_get_nonexistent_variable_fails(db):
    with pytest.raises(RuntimeError):
        db.execute("get foo")


def test_create_insert_read(db):
    db.execute("create foo u32")

    src = np.arange(5, dtype=np.uint32)
    db.execute(f"insert foo 0 {src.size}", src)

    dest = np.zeros(5, dtype=np.uint32)
    n = db.execute(f"read foo[0:] blimit {dest.nbytes}", dest)

    assert n == dest.size
    np.testing.assert_array_equal(dest, src)


def test_transaction_commit(db):
    db.execute("create foo u32")
    src = np.arange(3, dtype=np.uint32)

    with db.begin() as txn:
        txn.execute(f"insert foo 0 {src.size}", src)

    dest = np.zeros(3, dtype=np.uint32)
    n = db.execute(f"read foo[0:] blimit {dest.nbytes}", dest)
    assert n == dest.size
    np.testing.assert_array_equal(dest, src)


def test_transaction_rollback(db):
    db.execute("create foo u32")
    src = np.arange(3, dtype=np.uint32)

    txn = db.begin()
    txn.execute(f"insert foo 0 {src.size}", src)
    txn.rollback()

    dest = np.zeros(3, dtype=np.uint32)
    n = db.execute(f"read foo[0:] blimit {dest.nbytes}", dest)
    assert n == 0


def test_reusing_closed_transaction_raises(db):
    db.execute("create foo u32")

    txn = db.begin()
    txn.commit()

    with pytest.raises(RuntimeError):
        txn.execute("read foo[0:] blimit 4", np.zeros(1, dtype=np.uint32))


def test_double_commit_raises(db):
    txn = db.begin()
    txn.commit()

    with pytest.raises(RuntimeError):
        txn.commit()


def test_double_rollback_raises(db):
    txn = db.begin()
    txn.rollback()

    with pytest.raises(RuntimeError):
        txn.rollback()


def test_rollback_after_commit_raises(db):
    txn = db.begin()
    txn.commit()

    with pytest.raises(RuntimeError):
        txn.rollback()


def test_commit_after_rollback_raises(db):
    txn = db.begin()
    txn.rollback()

    with pytest.raises(RuntimeError):
        txn.commit()


def test_with_block_over_a_hand_committed_transaction_is_fine(db):
    db.execute("create foo u32")

    # Leaving the block is not a second commit - it has nothing left to do
    with db.begin() as txn:
        txn.execute("insert foo 0 3", np.arange(3, dtype=np.uint32))
        txn.commit()

    np.testing.assert_array_equal(db.execute("read foo[0:]"), np.arange(3, dtype=np.uint32))


def test_with_block_over_a_hand_rolled_back_transaction_is_fine(db):
    db.execute("create foo u32")

    with db.begin() as txn:
        txn.execute("insert foo 0 3", np.arange(3, dtype=np.uint32))
        txn.rollback()

    assert db.get("foo").length() == 0


def test_read_with_no_buffer_allocates_array(db):
    db.execute("create foo u32")
    src = np.arange(5, dtype=np.uint32)
    db.execute(f"insert foo 0 {src.size}", src)

    result = db.execute("read foo[0:]")

    assert isinstance(result, np.ndarray)
    assert result.dtype == np.uint32
    np.testing.assert_array_equal(result, src)


def test_remove_with_no_buffer_returns_removed_data(db):
    db.execute("create foo u32")
    src = np.arange(5, dtype=np.uint32)
    db.execute(f"insert foo 0 {src.size}", src)

    removed = db.execute("remove foo[0:2]")
    np.testing.assert_array_equal(removed, src[0:2])

    remaining = db.execute("read foo[0:]")
    np.testing.assert_array_equal(remaining, src[2:])


def test_var_basic_accessors(db):
    db.execute("create foo u32")

    var = db.get("foo")
    assert var.name() == "foo"
    assert var.type() == "u32"
    assert var.tsize() == 4
    assert var.length() == 0
    assert var.dtype() == np.dtype(np.uint32)


def test_var_length_tracks_inserts(db):
    db.execute("create foo u32")
    assert db.get("foo").length() == 0

    src = np.arange(5, dtype=np.uint32)
    db.execute(f"insert foo 0 {src.size}", src)

    assert db.get("foo").length() == src.size


def test_var_is_a_snapshot(db):
    db.execute("create foo u32")
    var = db.get("foo")

    src = np.arange(3, dtype=np.uint32)
    db.execute(f"insert foo 0 {src.size}", src)

    # Captured before the insert, so it still reports the old length
    assert var.length() == 0
    assert db.get("foo").length() == 3


def test_var_has_no_free_to_call(db):
    db.execute("create foo u32")

    # Releasing is the capsule destructor's job - nothing is exposed for it
    assert not hasattr(db.get("foo"), "free")
    assert not hasattr(ns._ns, "pyns_var_free")


def test_dropped_vars_are_released(db):
    db.execute("create foo f64")

    # Capturing and dropping without ever freeing must stay flat. If the
    # destructor were not running, each iteration would strand an arena.
    for _ in range(20000):
        assert db.get("foo").tsize() == 8


def test_var_repr(db):
    db.execute("create foo u32")
    assert repr(db.get("foo")) == "<Var foo type='u32' length=0 tsize=4>"


def test_var_name(db):
    db.execute("create foo u32")
    db.execute("create other_variable f64")

    assert db.get("foo").name() == "foo"
    assert db.get("other_variable").name() == "other_variable"


def test_var_name_from_create(db):
    assert db.execute("create foo u32").name() == "foo"


def test_var_of_nonexistent_variable_raises(db):
    with pytest.raises(RuntimeError):
        db.get("nope")


def test_var_composite_type(db):
    db.execute("create foo struct { a u32, b f64 }")

    var = db.get("foo")
    assert var.type() == "struct { a u32, b f64 }"
    assert var.tsize() == 12
    assert var.length() == 0


def test_var_in_transaction(db):
    db.execute("create foo u32")
    src = np.arange(4, dtype=np.uint32)

    with db.begin() as txn:
        txn.execute(f"insert foo 0 {src.size}", src)
        # The insert is visible inside its own transaction
        assert txn.get("foo").length() == src.size

    assert db.get("foo").length() == src.size


def test_var_after_database_close_raises(tmp_path):
    db = ns.Database(str(tmp_path / "varclose.db"))
    db.execute("create foo u32")
    db.close()

    with pytest.raises(RuntimeError):
        db.get("foo")


def test_execute_returns_none_when_nothing_is_produced(db):
    db.execute("create foo u32")
    # delete resolves no variable and allocates no data, so there is nothing
    # for the plan to hand back
    assert db.execute("delete foo") is None


def test_execute_returns_var_for_get(db):
    db.execute("create foo u32")

    var = db.execute("get foo")
    assert isinstance(var, ns.Var)
    assert var.type() == "u32"


def test_execute_returns_var_for_create(db):
    # create resolves the variable it just made, so the plan captures it
    var = db.execute("create foo u32")
    assert isinstance(var, ns.Var)
    assert var.type() == "u32"
    assert var.length() == 0


def test_execute_returns_array_for_read(db):
    db.execute("create foo u32")
    src = np.arange(3, dtype=np.uint32)
    db.execute(f"insert foo 0 {src.size}", src)

    assert isinstance(db.execute("read foo[0:]"), np.ndarray)


def test_execute_returns_count_when_given_a_buffer(db):
    db.execute("create foo u32")
    src = np.arange(3, dtype=np.uint32)
    assert db.execute(f"insert foo 0 {src.size}", src) == 3

    dest = np.zeros(3, dtype=np.uint32)
    assert db.execute(f"read foo[0:] blimit {dest.nbytes}", dest) == 3


def test_read_bounded_by_buffer_without_blimit(db):
    db.execute("create foo u32")
    src = np.arange(10, dtype=np.uint32)
    db.execute(f"insert foo 0 {src.size}", src)

    dest = np.zeros(4, dtype=np.uint32)
    assert db.execute("read foo[0:]", dest) == dest.size
    np.testing.assert_array_equal(dest, src[:4])


def test_sarray_variable_accepts_matching_2d_array(db):
    db.execute("create grid [3] u32")

    src = np.arange(6, dtype=np.uint32).reshape(2, 3)
    assert db.execute("insert grid 0 2", src) == 2
    np.testing.assert_array_equal(db.execute("read grid[0:]"), src)


def test_insert_into_nonexistent_variable_raises(db):
    with pytest.raises(RuntimeError):
        db.execute("insert nope 0 1", np.zeros(1, dtype=np.uint32))
