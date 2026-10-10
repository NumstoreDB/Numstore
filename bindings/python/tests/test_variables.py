from __future__ import annotations

import numpy as np
import pytest

import pynumstore as ns


def _cplx(part):
    return np.dtype([("re", part), ("im", part)])


S = np.dtype([("a", np.uint32), ("b", np.float32)])                    # 8 bytes
SA = np.dtype([("a", np.uint8), ("b", np.dtype((np.float32, (4,))))])  # 17 bytes, packed
U = np.dtype({
    "names": ["a", "b"],
    "formats": [np.uint32, np.float64],
    "offsets": [0, 0],
    "itemsize": 8,
})


# tsize is written out literally so it pins numstore's sizes independently
# of numpy's itemsize arithmetic
@pytest.mark.parametrize("vname, vtype, dtype, dims, tsize", [
    ("foo", "u32", np.uint32, (), 4),

    # arrays of primitives: leading dims move into the shape
    ("foo", "[10]u32", np.uint32, (10,), 40),
    ("foo", "[10][20]u32", np.uint32, (10, 20), 800),
    ("foo", "[1]u32", np.uint32, (1,), 4),                # not the same as scalar

    # structs
    ("foo", "struct { a u32, b f32 }", S, (), 8),
    ("foo", "[10][20]struct { a u32, b f32 }", S, (10, 20), 1600),

    # arrays inside a struct stay in the dtype and are NOT hoisted into shape
    ("foo", "struct { a u8, b [4]f32 }", SA, (), 17),
    ("foo", "[3]struct { a u8, b [4]f32 }", SA, (3,), 51),

    # unions: size of the largest member
    ("foo", "union { a u32, b f64 }", U, (), 8),
    ("foo", "[2]union { a u32, b f64 }", U, (2,), 16),

    # complex / wide primitives
    ("foo", "cu16", _cplx(np.uint8), (), 2),
    ("foo", "cf64", np.complex64, (), 8),
    ("foo", "[8]cf128", np.complex128, (8,), 128),

    # names
    ("_x", "u8", np.uint8, (), 1),
    ("foo_1", "u8", np.uint8, (), 1),
])
def test_capture_var(db, vname, vtype, dtype, dims, tsize):
    db.execute(f"create {vname} {vtype}")

    var = db.get(f"get {vname}")
    assert var == ns.Var(vname, vtype, 0, tsize)
    assert var.shape == (0, *dims)
    assert var.dtype == np.dtype(dtype)

def test_f128_var(db):
    db.execute("create foo f128")
    var = db.get("get foo")
    assert var == ns.Var("foo", "f128", 0, 16)

    if np.dtype(np.longdouble).itemsize == 16:
        assert var.dtype == np.dtype(np.longdouble)
    else:
        with pytest.raises(NotImplementedError):
            var.dtype


@pytest.mark.parametrize("vtype", [
    "", "u7", "cu8", "x32", "[0]u32", "[x]u32", "[10]",
    # "struct { }", "struct { a u8, a u8 }", "struct { a u8", "u32 junk",
])
def test_create_bad_type(db, vtype):
    with pytest.raises(RuntimeError):
        db.execute(f"create foo {vtype}")
    with pytest.raises(RuntimeError):    # failed create leaves nothing behind
        db.get("get foo")


@pytest.mark.parametrize("vname", ["1foo", "foo-bar", "[10]", "{"])
def test_create_bad_name(db, vname):
    with pytest.raises(RuntimeError):
        db.execute(f"create {vname} u32")


def test_create_duplicate(db):
    db.execute("create foo u32")
    with pytest.raises(RuntimeError):
        db.execute("create foo f64")
    assert db.get("get foo") == ns.Var("foo", "u32", 0, 4)   # original untouched


def test_multiple_vars_independent(db):
    db.execute("create a u8")
    db.execute("create b [4]f32")

    a, b = db.get("get a"), db.get("get b")
    assert a == ns.Var("a", "u8", 0, 1)
    assert b == ns.Var("b", "[4]f32", 0, 16)
    assert b.shape == (0, 4)
    assert b.dtype == np.dtype(np.float32)
