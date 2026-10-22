from __future__ import annotations

import numpy as np
import pytest

import pynumstore as ns


############# Empty and single-element boundaries
@pytest.mark.xfail(
    reason="zero-length operations are rejected by the query layer, not "
    "treated as no-ops. See the group of xfails in this file.",
    strict=True,
)
def test_zero_length_insert_writes_nothing(db):
    db.execute("create foo u32")

    assert db.write("insert foo 0 0", np.zeros(0, dtype=np.uint32)) == 0
    assert db.read("read foo[0:]").size == 0


@pytest.mark.xfail(
    reason="zero-length operations are rejected by the query layer, not "
    "treated as no-ops. See the group of xfails in this file.",
    strict=True,
)
def test_read_an_empty_range_at_the_end(db):
    db.execute("create foo u32")
    db.write("insert foo 0 3", np.arange(3, dtype=np.uint32))

    assert db.read("read foo[3:3]").size == 0


def test_read_a_single_element_range(db):
    db.execute("create foo u32")
    db.write("insert foo 0 5", np.arange(5, dtype=np.uint32))

    np.testing.assert_array_equal(db.read("read foo[4:5]"), [4])


############# Stride boundaries
def test_stride_larger_than_length_yields_only_the_first(db):
    db.execute("create foo u32")
    db.write("insert foo 0 4", np.arange(4, dtype=np.uint32))

    np.testing.assert_array_equal(db.read("read foo[0::100]"), [0])


def test_stride_equal_to_length(db):
    db.execute("create foo u32")
    db.write("insert foo 0 4", np.arange(4, dtype=np.uint32))

    np.testing.assert_array_equal(db.read("read foo[0::4]"), [0])


def test_stride_of_one_is_every_element(db):
    db.execute("create foo u32")
    src = np.arange(4, dtype=np.uint32)
    db.write("insert foo 0 4", src)

    np.testing.assert_array_equal(db.read("read foo[0::1]"), src)


def test_stride_of_zero_raises(db):
    db.execute("create foo u32")
    db.write("insert foo 0 4", np.arange(4, dtype=np.uint32))

    with pytest.raises(Exception):
        db.read("read foo[0::0]")


############# Numeric limits
def test_u8_holds_its_maximum(db):
    db.execute("create foo u8")
    src = np.array([0, 255], dtype=np.uint8)
    db.write("insert foo 0 2", src)

    np.testing.assert_array_equal(db.read("read foo[0:]"), src)


def test_u64_holds_its_maximum(db):
    db.execute("create foo u64")
    src = np.array([2**64 - 1], dtype=np.uint64)
    db.write("insert foo 0 1", src)

    np.testing.assert_array_equal(db.read("read foo[0:]"), src)


def test_i64_holds_its_minimum(db):
    db.execute("create foo i64")
    src = np.array([-(2**63)], dtype=np.int64)
    db.write("insert foo 0 1", src)

    np.testing.assert_array_equal(db.read("read foo[0:]"), src)


def test_f64_roundtrips_nan_and_infinities(db):
    db.execute("create foo f64")
    src = np.array([np.nan, np.inf, -np.inf], dtype=np.float64)
    db.write("insert foo 0 3", src)

    result = db.read("read foo[0:]")
    assert np.isnan(result[0])
    assert result[1] == np.inf
    assert result[2] == -np.inf


############# Insert position boundaries
def test_append_at_exactly_the_end(db):
    db.execute("create foo u32")
    db.write("insert foo 0 3", np.arange(3, dtype=np.uint32))

    db.write("insert foo 3 1", np.array([9], dtype=np.uint32))

    np.testing.assert_array_equal(db.read("read foo[0:]"), [0, 1, 2, 9])


@pytest.mark.xfail(
    reason="out-of-range positions clamp instead of raising: insert at 10 "
    "into a 2-element variable appends, giving length 3 and [0 1 9]. Same "
    "clamping as an oversized insert count.",
    strict=True,
)
def test_insert_leaving_a_gap_past_the_end_raises(db):
    db.execute("create foo u32")
    db.write("insert foo 0 2", np.arange(2, dtype=np.uint32))

    with pytest.raises(Exception):
        db.write("insert foo 10 1", np.array([9], dtype=np.uint32))


############# Remove boundaries
def test_removing_everything_leaves_an_empty_variable(db):
    db.execute("create foo u32")
    db.write("insert foo 0 3", np.arange(3, dtype=np.uint32))

    db.read("remove foo[0:3]")

    assert db.read("read foo[0:]").size == 0


@pytest.mark.xfail(
    reason="zero-length operations are rejected by the query layer, not "
    "treated as no-ops. See the group of xfails in this file.",
    strict=True,
)
def test_removing_an_empty_range_changes_nothing(db):
    db.execute("create foo u32")
    src = np.arange(3, dtype=np.uint32)
    db.write("insert foo 0 3", src)

    db.read("remove foo[1:1]")

    np.testing.assert_array_equal(db.read("read foo[0:]"), src)


############# read_into buffer edges
@pytest.mark.xfail(
    reason="zero-length operations are rejected by the query layer, not "
    "treated as no-ops. See the group of xfails in this file.",
    strict=True,
)
def test_read_into_a_zero_sized_buffer_reads_nothing(db):
    db.execute("create foo u32")
    db.write("insert foo 0 3", np.arange(3, dtype=np.uint32))

    assert db.read_into("read foo[0:]", np.zeros(0, dtype=np.uint32)) == 0


def test_read_into_an_oversized_buffer_leaves_the_tail_untouched(db):
    db.execute("create foo u32")
    db.write("insert foo 0 2", np.array([1, 2], dtype=np.uint32))

    dest = np.full(5, 77, dtype=np.uint32)
    assert db.read_into("read foo[0:]", dest) == 2
    np.testing.assert_array_equal(dest[2:], [77, 77, 77])


def test_read_into_the_wrong_dtype_raises(db):
    db.execute("create foo u32")
    db.write("insert foo 0 2", np.array([1, 2], dtype=np.uint32))

    with pytest.raises(TypeError):
        db.read_into("read foo[0:]", np.zeros(2, dtype=np.float64))


def test_read_into_a_noncontiguous_buffer_raises(db):
    db.execute("create foo u32")
    db.write("insert foo 0 2", np.array([1, 2], dtype=np.uint32))

    with pytest.raises(ValueError):
        db.read_into("read foo[0:]", np.zeros(10, dtype=np.uint32)[::2])


def test_read_into_a_readonly_buffer_raises(db):
    db.execute("create foo u32")
    db.write("insert foo 0 2", np.array([1, 2], dtype=np.uint32))

    dest = np.zeros(2, dtype=np.uint32)
    dest.flags.writeable = False
    with pytest.raises(ValueError):
        db.read_into("read foo[0:]", dest)


def test_read_into_a_list_raises(db):
    db.execute("create foo u32")
    db.write("insert foo 0 2", np.array([1, 2], dtype=np.uint32))

    with pytest.raises(TypeError):
        db.read_into("read foo[0:]", [0, 0])
