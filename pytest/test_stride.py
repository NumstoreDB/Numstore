"""Stride semantics shared by READ, WRITE and REMOVE.

A bracketed range without a colon is a bare index and covers exactly one
element. `stride_resolve` used to ignore the parser's COLON_PRESENT flag, so a
missing stop always meant "to the end" and `foo[2]` silently behaved like
`foo[2:]`. The tests below pin the distinction down for every operation that
takes a range.
"""

from __future__ import annotations

import numpy as np
import pytest

import pynumstore as ns


@pytest.fixture
def foo(db):
    """A u32 variable holding [0, 1, 2, 3, 4, 5]."""
    db.execute("create foo u32")
    db.write("insert foo 0 6", np.arange(6, dtype=np.uint32))
    return db


############# A bare index covers exactly one element
@pytest.mark.parametrize("index,expected", [(0, 0), (2, 2), (5, 5)])
def test_read_a_bare_index_is_one_element(foo, index, expected):
    np.testing.assert_array_equal(foo.read(f"read foo[{index}]"), [expected])


def test_read_a_bare_negative_index_is_one_element(foo):
    np.testing.assert_array_equal(foo.read("read foo[-1]"), [5])
    np.testing.assert_array_equal(foo.read("read foo[-6]"), [0])


def test_a_bare_index_is_not_an_open_range(foo):
    """The regression itself: [2] and [2:] must not agree."""
    bare = foo.read("read foo[2]")
    open_ended = foo.read("read foo[2:]")

    assert bare.size == 1
    assert open_ended.size == 4
    np.testing.assert_array_equal(bare, open_ended[:1])


def test_a_bare_index_past_the_end_is_an_empty_range(foo):
    assert foo.read_into("read foo[6]", np.zeros(6, dtype=np.uint32)) == 0
    assert foo.read_into("read foo[99]", np.zeros(6, dtype=np.uint32)) == 0


def test_a_bare_index_on_a_strict_array_is_one_element(db):
    db.execute("create grid [3] u32")
    db.write("insert grid 0 3", np.arange(9, dtype=np.uint32).reshape(3, 3))

    assert db.read("read grid[1]").shape == (1, 3)
    np.testing.assert_array_equal(db.read("read grid[1]"), [[3, 4, 5]])
    assert db.read("read grid[1:]").shape == (2, 3)


############# The same rule for WRITE
def test_write_to_a_bare_index_touches_one_element(foo):
    assert foo.write("write foo[2]", [70, 80, 90]) == 1
    np.testing.assert_array_equal(foo.read("read foo"), [0, 1, 70, 3, 4, 5])


def test_write_to_an_open_range_touches_the_rest(foo):
    assert foo.write("write foo[2:]", [70, 80, 90]) == 3
    np.testing.assert_array_equal(foo.read("read foo"), [0, 1, 70, 80, 90, 5])


def test_write_to_a_bare_index_past_the_end_is_a_noop(foo):
    assert foo.write("write foo[6]", [70]) == 0
    np.testing.assert_array_equal(foo.read("read foo"), np.arange(6))


############# And for REMOVE
def test_remove_a_bare_index_drops_one_element(foo):
    foo.execute("remove foo[2]")

    np.testing.assert_array_equal(foo.read("read foo"), [0, 1, 3, 4, 5])
    assert foo.get("get foo").length == 5


def test_remove_an_open_range_drops_the_rest(foo):
    foo.execute("remove foo[2:]")

    np.testing.assert_array_equal(foo.read("read foo"), [0, 1])
    assert foo.get("get foo").length == 2


def test_destructive_read_of_a_bare_index_returns_one_element(foo):
    np.testing.assert_array_equal(foo.read("remove foo[2]"), [2])
    np.testing.assert_array_equal(foo.read("read foo"), [0, 1, 3, 4, 5])


############# Colon forms keep their old meaning
def test_an_explicit_single_element_range_still_works(foo):
    np.testing.assert_array_equal(foo.read("read foo[2:3]"), [2])


@pytest.mark.parametrize("rng", ["", "[:]", "[0:]", "[0::1]"])
def test_whole_array_forms_agree(foo, rng):
    np.testing.assert_array_equal(foo.read(f"read foo{rng}"), np.arange(6))


############# Ranges clamp rather than raise
@pytest.mark.parametrize("rng", ["[0:99]", "[-99:]", "[-99:99]"])
def test_an_oversized_range_clamps_to_the_array(foo, rng):
    np.testing.assert_array_equal(foo.read(f"read foo{rng}"), np.arange(6))


def test_a_start_past_the_end_is_an_empty_range(foo):
    assert foo.read_into("read foo[7:]", np.zeros(6, dtype=np.uint32)) == 0


############# Steps
def test_a_step_spreads_the_range_out(foo):
    np.testing.assert_array_equal(foo.read("read foo[0:6:2]"), [0, 2, 4])


@pytest.mark.parametrize("step", [0, -1, -2])
def test_a_non_positive_step_is_rejected(foo, step):
    with pytest.raises(RuntimeError, match="stride step must be positive"):
        foo.read(f"read foo[0:6:{step}]")
