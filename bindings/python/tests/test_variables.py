from __future__ import annotations

import dataclasses

import numpy as np
import pytest

import pynumstore as ns

import math

import numpy as np
import pytest


def _var(name, vtype, base, dims=()):
    base = np.dtype(base)
    dims = tuple(dims)
    return Var(name, vtype, 0, base.itemsize * math.prod(dims), (0, *dims), base, dims)


S = np.dtype([("a", np.uint32), ("b", np.float32)])                # 8 bytes
SA = np.dtype([("a", np.uint8), ("b", np.dtype((np.float32, (4,))))])  # 17 bytes, packed
U = np.dtype({
    "names": ["a", "b"],
    "formats": [np.uint32, np.float64],
    "offsets": [0, 0],
    "itemsize": 8,
})

@pytest.mark.parametrize("vname, vtype, exp", [
    # pin the helper with one fully literal row
    ("foo", "u32", Var("foo", "u32", 0, 4, (0,), np.dtype(np.uint32), ())),

    # arrays of primitives
    ("foo", "[10]u32", _var("foo", "[10]u32", np.uint32, (10,))),           # 40
    ("foo", "[10][20]u32", _var("foo", "[10][20]u32", np.uint32, (10, 20))),  # 800
    ("foo", "[1]u32", _var("foo", "[1]u32", np.uint32, (1,))),              # not the same as scalar

    # structs
    ("foo", "struct { a u32, b f32 }", _var("foo", "struct { a u32, b f32 }", S)),
    ("foo", "[10][20]struct { a u32, b f32 }",
     _var("foo", "[10][20]struct { a u32, b f32 }", S, (10, 20))),        # 1600

    # arrays inside a struct stay in the dtype and are NOT hoisted into shape
    ("foo", "struct { a u8, b [4]f32 }", _var("foo", "struct { a u8, b [4]f32 }", SA)),
    ("foo", "[3]struct { a u8, b [4]f32 }", _var("foo", "[3]struct { a u8, b [4]f32 }", SA, (3,))),              # 51

    # unions: size of the largest member
    ("foo", "union { a u32, b f64 }", _var("foo", "union { a u32, b f64 }", U)),
    ("foo", "[2]union { a u32, b f64 }", _var("foo", "[2]union { a u32, b f64 }", U, (2,))),

    # complex / wide primitives
    ("foo", "cu16", _var("foo", "cu16", _cplx(np.uint8))),
    ("foo", "cf64", _var("foo", "cf64", np.complex64)),
    ("foo", "[8]cf128", _var("foo", "[8]cf128", np.complex128, (8,))),
    ("foo", "f128", _var("foo", "f128", np.longdouble)),

    # names
    ("_x", "u8", _var("_x", "u8", np.uint8)),
    ("foo_1", "u8", _var("foo_1", "u8", np.uint8)),
])
def test_capture_var(db, vname, vtype, exp):
    db.execute(f"create {vname} {vtype}")
    assert db.get(vname) == exp


@pytest.mark.parametrize("vtype", [
    "", "u7", "cu8", "x32", "[0]u32", "[x]u32", "[10]",
    "struct { }", "struct { a u8, a u8 }", "struct { a u8", "u32 junk",
])
def test_create_bad_type(db, vtype):
    with pytest.raises(ValueError):
        db.execute(f"create foo {vtype}")
    with pytest.raises(KeyError):        # failed create leaves nothing behind
        db.get("foo")


@pytest.mark.parametrize("vname", ["1foo", "foo-bar", "[10]", "{"])
def test_create_bad_name(db, vname):
    with pytest.raises(ValueError):
        db.execute(f"create {vname} u32")


def test_create_duplicate(db):
    db.execute("create foo u32")
    with pytest.raises(ValueError):
        db.execute("create foo f64")
    assert db.get("foo") == _var("foo", "u32", np.uint32)   # original untouched


def test_get_missing(db):
    with pytest.raises(KeyError):
        db.get("nope")


def test_multiple_vars_independent(db):
    db.execute("create a u8")
    db.execute("create b [4]f32")
    assert db.get("a") == _var("a", "u8", np.uint8)
    assert db.get("b") == _var("b", "[4]f32", np.float32, (4,))

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

