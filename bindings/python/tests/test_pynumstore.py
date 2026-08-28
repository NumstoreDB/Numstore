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
