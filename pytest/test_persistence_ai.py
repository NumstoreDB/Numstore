from __future__ import annotations

import numpy as np

import pynumstore as ns


def test_committed_transaction_survives_reopen(db_dir):
    db = ns.Database("p.db")
    db.execute("create foo u32")
    with db.begin() as tx:
        tx.write("insert foo 0 2", np.array([4, 5], dtype=np.uint32))
    db.close()

    db = ns.Database("p.db")
    np.testing.assert_array_equal(db.read("read foo[0:]"), [4, 5])
    db.close()


def test_rolled_back_transaction_does_not_survive_reopen(db_dir):
    db = ns.Database("p.db")
    db.execute("create foo u32")
    tx = db.begin()
    tx.write("insert foo 0 2", np.array([4, 5], dtype=np.uint32))
    tx.rollback()
    db.close()

    db = ns.Database("p.db")
    assert db.read("read foo[0:]").size == 0
    db.close()


def test_reopen_without_closing_the_first_handle(db_dir):
    db = ns.Database("p.db")
    db.execute("create foo u32")
    db.write("insert foo 0 1", np.array([1], dtype=np.uint32))

    second = ns.Database("p.db")
    try:
        np.testing.assert_array_equal(second.read("read foo[0:]"), [1])
    finally:
        second.close()
        db.close()
