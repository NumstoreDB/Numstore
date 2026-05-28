"""Tests for numpy -> numstore conversion."""

import numpy as np
import pytest

from numstore import nstype_to_dtype, dtype_to_nstype

@pytest.mark.parametrize(("np_str", "expected"), [
    ("u1", "u8"),  ("u2", "u16"), ("u4", "u32"), ("u8", "u64"),
    ("i1", "i8"),  ("i2", "i16"), ("i4", "i32"), ("i8", "i64"),
    ("f2", "f16"), ("f4", "f32"), ("f8", "f64"),
    ("c8", "cf64"), ("c16", "cf128"),
])
def test_primitive(np_str, expected):
    assert dtype_to_nstype(np.dtype(np_str)) == expected


def test_f128_if_supported():
    """f128 is longdouble; not 16 bytes on Windows / some 32-bit platforms."""
    try:
        d = np.dtype(np.longdouble)
    except TypeError:
        pytest.skip("longdouble not available")
        return

    if d.itemsize != 16:
        pytest.skip(f"longdouble is {d.itemsize} bytes on this platform")

    assert dtype_to_nstype(d) == "f128"


def test_cf256_if_supported():
    try:
        d = np.dtype(np.clongdouble)
    except TypeError:
        pytest.skip("clongdouble not available")
        return

    if d.itemsize != 32:
        pytest.skip(f"clongdouble is {d.itemsize} bytes on this platform")

    assert dtype_to_nstype(d) == "cf256"

#############################################
##### Structs

def test_simple_struct():
    d = np.dtype([("foo", "i4"), ("bar", "f8")])
    assert dtype_to_nstype(d) == "struct { foo i32, bar f64 }"


def test_nested_struct():
    d = np.dtype([
        ("foo", "i4"),
        ("bar", [("biz", "u4"), ("buz", "f4")]),
    ])
    assert (
        dtype_to_nstype(d)
        == "struct { foo i32, bar struct { biz u32, buz f32 } }"
    )


def test_single_field_struct():
    d = np.dtype([("only", "i4")])
    assert dtype_to_nstype(d) == "struct { only i32 }"


#############################################
##### Unions

def test_simple_union():
    d = np.dtype({
        "names": ["a", "b"],
        "formats": ["i4", "f4"],
        "offsets": [0, 0],
        "itemsize": 4,
    })
    assert dtype_to_nstype(d) == "union { a i32, b f32 }"


def test_union_of_struct_and_prim():
    d = np.dtype({
        "names": ["x", "y"],
        "formats": ["i4", [("a", "u4"), ("b", "f4")]],
        "offsets": [0, 0],
        "itemsize": 8,
    })
    assert dtype_to_nstype(d) == "union { x i32, y struct { a u32, b f32 } }"

#############################################
##### Strict Arrays

def test_simple_array():
    assert dtype_to_nstype(np.dtype(("i4", (10,)))) == "[10] i32"


def test_multidim_array():
    assert dtype_to_nstype(np.dtype(("f8", (20, 30)))) == "[20][30] f64"


def test_array_of_struct():
    d = np.dtype(([("x", "i4"), ("y", "f4")], (5,)))
    assert dtype_to_nstype(d) == "[5] struct { x i32, y f32 }"


def test_struct_with_array_field():
    d = np.dtype([("xs", "i4", (10,)), ("y", "f8")])
    assert dtype_to_nstype(d) == "struct { xs [10] i32, y f64 }"

#############################################
##### Bigger Example

def test_spec_example():
    # [20][30] struct { a union { i f32, b i64 }, c [10] f64 }
    union_d = np.dtype({
        "names": ["i", "b"],
        "formats": ["f4", "i8"],
        "offsets": [0, 0],
        "itemsize": 8,
    })
    inner = np.dtype([("a", union_d), ("c", np.dtype(("f8", (10,))))])
    outer = np.dtype((inner, (20, 30)))
    assert (
        dtype_to_nstype(outer)
        == "[20][30] struct { a union { i f32, b i64 }, c [10] f64 }"
    )

#############################################
##### Failures

@pytest.mark.parametrize("np_str", [
    "O",       # object
    "U10",     # unicode string
    "S5",      # bytes string
    "M8[s]",   # datetime
    "m8[s]",   # timedelta
    "V8",      # unstructured void
    "?",       # bool
])
def test_unsupported(np_str):
    with pytest.raises(ValueError):
        dtype_to_nstype(np.dtype(np_str))

def test_unsupported_inside_struct_propagates():
    d = np.dtype([("ok", "i4"), ("bad", "O")])
    with pytest.raises(ValueError):
        dtype_to_nstype(d)
