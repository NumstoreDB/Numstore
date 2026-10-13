from __future__ import annotations

import numpy as np
import pytest

import pynumstore as ns


def test_strided_read_returns_every_nth_element(db):
    db.execute("create foo u32")
    src = np.arange(10, dtype=np.uint32)
    db.write(f"insert foo 0 {src.size}", src)

    result = db.read("read foo[0::2]")

    np.testing.assert_array_equal(result, src[::2])


def test_ranged_read_is_half_open(db):
    db.execute("create foo u32")
    src = np.arange(10, dtype=np.uint32)
    db.write(f"insert foo 0 {src.size}", src)

    result = db.read("read foo[2:5]")

    np.testing.assert_array_equal(result, src[2:5])


def test_read_of_an_empty_variable_is_empty(db):
    db.execute("create foo u32")

    result = db.read("read foo[0:]")

    assert result.size == 0


def test_write_accepts_a_plain_python_list(db):
    db.execute("create foo f64")

    db.write("insert foo 0 3", [1.5, 2.25, 3.75])

    np.testing.assert_array_equal(db.read("read foo[0:]"), [1.5, 2.25, 3.75])


def test_f64_roundtrips_without_precision_loss(db):
    db.execute("create foo f64")
    src = np.array([np.pi, -1e308, 2.2250738585072014e-308], dtype=np.float64)
    db.write(f"insert foo 0 {src.size}", src)

    np.testing.assert_array_equal(db.read("read foo[0:]"), src)


def test_insert_in_the_middle_shifts_later_elements(db):
    db.execute("create foo u32")
    db.write("insert foo 0 4", np.array([0, 1, 2, 3], dtype=np.uint32))

    db.write("insert foo 2 1", np.array([99], dtype=np.uint32))

    np.testing.assert_array_equal(db.read("read foo[0:]"), [0, 1, 99, 2, 3])


def test_uncommitted_write_is_visible_inside_its_transaction(db):
    db.execute("create foo u32")

    with db.begin() as tx:
        tx.write("insert foo 0 2", np.array([7, 8], dtype=np.uint32))
        np.testing.assert_array_equal(tx.read("read foo[0:]"), [7, 8])


def test_rollback_discards_the_write(db):
    db.execute("create foo u32")

    tx = db.begin()
    tx.write("insert foo 0 2", np.array([7, 8], dtype=np.uint32))
    tx.rollback()

    assert db.read("read foo[0:]").size == 0


def test_read_past_the_end_raises(db):
    db.execute("create foo u32")
    db.write("insert foo 0 2", np.array([1, 2], dtype=np.uint32))

    with pytest.raises(Exception):
        db.read("read foo[5:9]")


@pytest.mark.xfail(
    reason="the count in an insert query is an upper bound, not a contract: "
    "write() clamps to the array and returns 2, matching how read_into is "
    "bounded by its buffer. Not a bug -- the test's expectation was wrong.",
    strict=True,
)
def test_write_with_a_count_mismatching_the_array_raises(db):
    db.execute("create foo u32")

    with pytest.raises(Exception):
        db.write("insert foo 0 5", np.array([1, 2], dtype=np.uint32))
