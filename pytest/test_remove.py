from __future__ import annotations

import dataclasses

import numpy as np
import pytest

import pynumstore as ns


def test_read_of_a_remove_returns_the_removed_data(db):
    db.execute("create foo u32")
    src = np.arange(5, dtype=np.uint32)
    db.write(f"insert foo 0 {src.size}", src)

    removed = db.read("remove foo[0:2]")
    np.testing.assert_array_equal(removed, src[0:2])

    remaining = db.read("read foo[0:]")
    np.testing.assert_array_equal(remaining, src[2:])


############# Reported length after a remove
#
# numstore_remove and numstore_remove_malloc both copied insert's size update,
# where `len` is the number of elements moved. In remove, `len` is the length
# the array started with, so `nbytes + len * tsize` stored exactly twice the
# real size and GET reported 2x the pre-remove length while the data itself
# stayed correct.
def _length_matches(db, name, dtype):
    """GET's length, and the number of elements actually there."""
    reported = db.get(f"get {name}").length
    buf = np.zeros(max(reported, 1) * 2 + 8, dtype=dtype)
    return reported, db.read_into(f"read {name}", buf)


def test_get_length_after_a_partial_remove(db):
    db.execute("create foo u32")
    db.write("insert foo 0 10", np.arange(10, dtype=np.uint32))

    db.execute("remove foo[0:2]")

    assert _length_matches(db, "foo", np.uint32) == (8, 8)


def test_get_length_is_not_doubled_by_a_remove(db):
    """The exact shape of the bug: the stored size was nbytes * 2."""
    db.execute("create foo u32")
    db.write("insert foo 0 10", np.arange(10, dtype=np.uint32))

    db.execute("remove foo[0:1]")

    assert db.get("get foo").length == 9


def test_get_length_after_removing_everything(db):
    db.execute("create foo u32")
    db.write("insert foo 0 6", np.arange(6, dtype=np.uint32))

    db.execute("remove foo")

    assert _length_matches(db, "foo", np.uint32) == (0, 0)
    assert db.read("read foo").size == 0


def test_get_length_after_a_strided_remove(db):
    db.execute("create foo u32")
    db.write("insert foo 0 6", np.arange(6, dtype=np.uint32))

    db.execute("remove foo[0:6:2]")

    assert _length_matches(db, "foo", np.uint32) == (3, 3)


def test_get_length_after_an_empty_remove_is_unchanged(db):
    db.execute("create foo u32")
    db.write("insert foo 0 6", np.arange(6, dtype=np.uint32))

    db.execute("remove foo[6:]")

    assert _length_matches(db, "foo", np.uint32) == (6, 6)


def test_get_length_after_a_destructive_read(db):
    """db.read of a REMOVE goes through numstore_remove_malloc, not
    numstore_remove - a separate copy of the same size update."""
    db.execute("create foo f64")
    db.write("insert foo 0 5", np.arange(5, dtype=np.float64))

    np.testing.assert_array_equal(db.read("remove foo[0:2]"), [0.0, 1.0])

    assert _length_matches(db, "foo", np.float64) == (3, 3)


@pytest.mark.parametrize(
    "type_str,dtype", [("u8", np.uint8), ("u32", np.uint32), ("f64", np.float64)]
)
def test_get_length_after_a_remove_is_independent_of_element_size(db, type_str, dtype):
    db.execute(f"create foo {type_str}")
    db.write("insert foo 0 10", np.arange(10, dtype=dtype))

    db.execute("remove foo[0:4]")

    assert _length_matches(db, "foo", dtype) == (6, 6)


def test_repeated_removes_keep_the_length_consistent(db):
    db.execute("create foo u32")
    db.write("insert foo 0 10", np.arange(10, dtype=np.uint32))

    for expected in (8, 6, 4, 2, 0):
        db.execute("remove foo[0:2]")
        assert db.get("get foo").length == expected


def test_var_shape_after_a_remove(db):
    db.execute("create grid [3] u32")
    db.write("insert grid 0 4", np.arange(12, dtype=np.uint32).reshape(4, 3))

    db.execute("remove grid[0:2]")

    assert db.get("get grid").shape == (2, 3)
    assert db.read("read grid").shape == (2, 3)


def test_insert_after_a_remove_lands_at_the_right_offset(db):
    """A wrong stored length makes the next offset resolve against it."""
    db.execute("create foo u32")
    db.write("insert foo 0 6", np.arange(6, dtype=np.uint32))
    db.execute("remove foo[0:3]")

    db.write("insert foo 3 2", np.array([70, 80], dtype=np.uint32))

    np.testing.assert_array_equal(db.read("read foo"), [3, 4, 5, 70, 80])
    assert db.get("get foo").length == 5


def test_get_length_after_a_remove_survives_reopen(db_dir):
    db = ns.Database("r.db")
    db.execute("create foo u32")
    db.write("insert foo 0 10", np.arange(10, dtype=np.uint32))
    db.execute("remove foo[0:4]")
    assert db.get("get foo").length == 6
    db.close()

    db = ns.Database("r.db")
    try:
        assert db.get("get foo").length == 6
        np.testing.assert_array_equal(db.read("read foo"), [4, 5, 6, 7, 8, 9])
    finally:
        db.close()


def test_get_length_after_a_remove_in_a_transaction(db):
    db.execute("create foo u32")
    db.write("insert foo 0 6", np.arange(6, dtype=np.uint32))

    with db.begin() as txn:
        txn.execute("remove foo[0:3]")
        assert txn.get("get foo").length == 3

    assert db.get("get foo").length == 3


def test_a_rolled_back_remove_restores_the_length(db):
    db.execute("create foo u32")
    db.write("insert foo 0 6", np.arange(6, dtype=np.uint32))

    txn = db.begin()
    txn.execute("remove foo[0:3]")
    txn.rollback()

    assert _length_matches(db, "foo", np.uint32) == (6, 6)
    np.testing.assert_array_equal(db.read("read foo"), np.arange(6))
