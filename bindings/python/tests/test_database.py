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

############# Execute 
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


############# Logic
def test_create_insert_read(db):
    db.execute("create foo u32")

    src = np.arange(5, dtype=np.uint32)
    db.write(f"insert foo 0 {src.size}", src)

    dest = np.zeros(5, dtype=np.uint32)
    n = db.read_into(f"read foo[0:] blimit {dest.nbytes}", dest)

    assert n == dest.size
    np.testing.assert_array_equal(dest, src)


def test_transaction_commit(db):
    db.execute("create foo u32")
    src = np.arange(3, dtype=np.uint32)

    with db.begin() as txn:
        txn.write(f"insert foo 0 {src.size}", src)

    dest = np.zeros(3, dtype=np.uint32)
    n = db.read_into(f"read foo[0:] blimit {dest.nbytes}", dest)
    assert n == dest.size
    np.testing.assert_array_equal(dest, src)


def test_transaction_rollback(db):
    db.execute("create foo u32")
    src = np.arange(3, dtype=np.uint32)

    txn = db.begin()
    txn.write(f"insert foo 0 {src.size}", src)
    txn.rollback()

    dest = np.zeros(3, dtype=np.uint32)
    n = db.read_into(f"read foo[0:] blimit {dest.nbytes}", dest)
    assert n == 0


def test_reusing_closed_transaction_raises(db):
    db.execute("create foo u32")

    txn = db.begin()
    txn.commit()

    with pytest.raises(RuntimeError):
        txn.read_into("read foo[0:] blimit 4", np.zeros(1, dtype=np.uint32))


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
        txn.write("insert foo 0 3", np.arange(3, dtype=np.uint32))
        txn.commit()

    np.testing.assert_array_equal(
        db.read("read foo[0:]"), np.arange(3, dtype=np.uint32)
    )


def test_with_block_over_a_hand_rolled_back_transaction_is_fine(db):
    db.execute("create foo u32")

    with db.begin() as txn:
        txn.write("insert foo 0 3", np.arange(3, dtype=np.uint32))
        txn.rollback()

    assert db.get("get foo").length == 0


def test_read_allocates_an_array(db):
    db.execute("create foo u32")
    src = np.arange(5, dtype=np.uint32)
    db.write(f"insert foo 0 {src.size}", src)

    result = db.read("read foo[0:]")

    assert isinstance(result, np.ndarray)
    assert result.dtype == np.uint32
    np.testing.assert_array_equal(result, src)


def test_read_of_a_remove_returns_the_removed_data(db):
    db.execute("create foo u32")
    src = np.arange(5, dtype=np.uint32)
    db.write(f"insert foo 0 {src.size}", src)

    removed = db.read("remove foo[0:2]")
    np.testing.assert_array_equal(removed, src[0:2])

    remaining = db.read("read foo[0:]")
    np.testing.assert_array_equal(remaining, src[2:])


def test_var_basic_accessors(db):
    db.execute("create foo u32")

    var = db.get("get foo")
    assert var.name == "foo"
    assert var.type == "u32"
    assert var.tsize == 4
    assert var.length == 0
    assert var.dtype == np.dtype(np.uint32)
    assert var.shape == (0,)
    assert len(var) == 0


def test_var_length_tracks_inserts(db):
    db.execute("create foo u32")
    assert db.get("get foo").length == 0

    src = np.arange(5, dtype=np.uint32)
    db.write(f"insert foo 0 {src.size}", src)

    assert db.get("get foo").length == src.size


def test_var_is_a_snapshot(db):
    db.execute("create foo u32")
    var = db.get("get foo")

    src = np.arange(3, dtype=np.uint32)
    db.write(f"insert foo 0 {src.size}", src)

    # Captured before the insert, so it still reports the old length
    assert var.length == 0
    assert db.get("get foo").length == 3


def test_var_holds_no_c_handle(db):
    db.execute("create foo u32")

    # _capture_var frees the handle before returning, so a Var is a plain
    # value with nothing to release - and nothing is exposed to release it
    var = db.get("get foo")
    assert not hasattr(var, "free")
    assert {f.name for f in dataclasses.fields(var)} == {
        "name",
        "type",
        "length",
        "tsize",
    }

def test_dropped_vars_are_released(db):
    db.execute("create foo f64")

    # Capturing and dropping without ever freeing must stay flat. If the
    # destructor were not running, each iteration would strand an arena.
    for _ in range(20000):
        assert db.get("get foo").tsize == 8


def test_var_repr(db):
    db.execute("create foo u32")
    assert repr(db.get("get foo")) == "Var(name='foo', type='u32', length=0, tsize=4)"


def test_var_name(db):
    db.execute("create foo u32")
    db.execute("create other_variable f64")

    assert db.get("get foo").name == "foo"
    assert db.get("get other_variable").name == "other_variable"


def test_var_of_nonexistent_variable_raises(db):
    with pytest.raises(RuntimeError):
        db.get("get nope")


def test_var_composite_type(db):
    db.execute("create foo struct { a u32, b f64 }")

    var = db.get("get foo")
    assert var.type == "struct { a u32, b f64 }"
    assert var.tsize == 12
    assert var.length == 0


def test_var_in_transaction(db):
    db.execute("create foo u32")
    src = np.arange(4, dtype=np.uint32)

    with db.begin() as txn:
        txn.write(f"insert foo 0 {src.size}", src)
        # The insert is visible inside its own transaction
        assert txn.get("get foo").length == src.size

    assert db.get("get foo").length == src.size


def test_var_after_database_close_raises(db_dir):
    db = ns.Database("varclose.db")
    db.execute("create foo u32")
    db.close()

    with pytest.raises(RuntimeError):
        db.get("get foo")


def test_execute_returns_a_count(db):
    assert isinstance(db.execute("create foo u32"), int)
    assert isinstance(db.execute("delete foo"), int)


def test_get_returns_a_var(db):
    db.execute("create foo u32")

    var = db.get("get foo")
    assert isinstance(var, ns.Var)
    assert var.type == "u32"


def test_write_returns_the_count_written(db):
    db.execute("create foo u32")
    src = np.arange(3, dtype=np.uint32)
    assert db.write(f"insert foo 0 {src.size}", src) == src.size


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


def test_sarray_variable_accepts_matching_2d_array(db):
    db.execute("create grid [3] u32")

    src = np.arange(6, dtype=np.uint32).reshape(2, 3)
    assert db.write("insert grid 0 2", src) == 2
    np.testing.assert_array_equal(db.read("read grid[0:]"), src)


def test_insert_into_nonexistent_variable_raises(db):
    with pytest.raises(RuntimeError):
        db.write("insert nope 0 1", np.zeros(1, dtype=np.uint32))
