from __future__ import annotations

import dataclasses

import numpy as np
import pytest

import pynumstore as ns

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

def test_var_in_transaction(db):
    db.execute("create foo u32")
    src = np.arange(4, dtype=np.uint32)

    with db.begin() as txn:
        txn.write(f"insert foo 0 {src.size}", src)
        assert txn.get("get foo").length == src.size

    assert db.get("get foo").length == src.size
