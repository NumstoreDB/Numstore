from __future__ import annotations

import numpy as np
import pytest

import pynumstore as ns

def _cplx(part):
    return np.dtype([("re", part), ("im", part)])

def test_primitive_to_dtype():
    # real
    assert ns.to_dtype("u8") == np.dtype(np.uint8)
    assert ns.to_dtype("u16") == np.dtype(np.uint16)
    assert ns.to_dtype("u32") == np.dtype(np.uint32)
    assert ns.to_dtype("u64") == np.dtype(np.uint64)

    assert ns.to_dtype("i8") == np.dtype(np.int8)
    assert ns.to_dtype("i16") == np.dtype(np.int16)
    assert ns.to_dtype("i32") == np.dtype(np.int32)
    assert ns.to_dtype("i64") == np.dtype(np.int64)

    assert ns.to_dtype("f16") == np.dtype(np.float16)
    assert ns.to_dtype("f32") == np.dtype(np.float32)
    assert ns.to_dtype("f64") == np.dtype(np.float64)

    # complex unsigned: total bits = 2 * part bits
    assert ns.to_dtype("cu16") == _cplx(np.uint8)
    assert ns.to_dtype("cu32") == _cplx(np.uint16)
    assert ns.to_dtype("cu64") == _cplx(np.uint32)
    assert ns.to_dtype("cu128") == _cplx(np.uint64)

    # complex signed
    assert ns.to_dtype("ci16") == _cplx(np.int8)
    assert ns.to_dtype("ci32") == _cplx(np.int16)
    assert ns.to_dtype("ci64") == _cplx(np.int32)
    assert ns.to_dtype("ci128") == _cplx(np.int64)

    # complex float: no native complex32, so structured; the rest are native
    assert ns.to_dtype("cf32") == _cplx(np.float16)
    assert ns.to_dtype("cf64") == np.dtype(np.complex64)
    assert ns.to_dtype("cf128") == np.dtype(np.complex128)

    # Only run on longdouble support
    longdouble_ok = np.dtype(np.longdouble).itemsize == 16
    if longdouble_ok:
        assert ns.to_dtype("f128") == np.dtype(np.longdouble)
        assert ns.to_dtype("cf256") == np.dtype(np.clongdouble)
    else:
        for code in ["f128", "cf256"]:
            with pytest.raises(NotImplementedError):
                ns.to_dtype(code)

    # sanity: total bits really is total bits
    for code in ["cu16", "ci32", "cf32", "cf64", "cf128"]:
        assert ns.to_dtype(code).itemsize * 8 == int(code[2:])

    for bad in ["cu8", "ci8", "cf16", "u7", "f8", "x32", "c", ""]:
        with pytest.raises(ValueError):
            ns.to_dtype(bad)

def test_more_to_dtype():
    assert ns.to_dtype("struct { i u8, b u32 }") == np.dtype(
        [("i", np.uint8), ("b", np.uint32)]
    )

    assert ns.to_dtype("struct { i u8, b struct { a u32, b f32 } }") == np.dtype(
        [("i", np.uint8), ("b", [("a", np.uint32), ("b", np.float32)])]
    )

    assert ns.to_dtype("struct { i u8, b struct { a u32, b [10]f32 } }") == np.dtype(
        [("i", np.uint8), ("b", [("a", np.uint32), ("b", np.float32, (10,))])]
    )

    inner = np.dtype([("a", np.uint32), ("b", np.float32, (10, 20))])
    assert ns.to_dtype("struct { i u8, b struct { a u32, b [10][20]f32 } }") == np.dtype(
        [("i", np.uint8), ("b", inner)]
    )

    union = np.dtype({
        "names": ["i", "b"],
        "formats": [np.uint8, inner],
        "offsets": [0, 0],
        "itemsize": inner.itemsize,  # 4 + 10*20*4 = 804
    })
    assert ns.to_dtype("union { i u8, b struct { a u32, b [10][20]f32 } }") == union
    assert union.itemsize == 804

    assert ns.to_dtype(
        "[10][20]union { i u8, b struct { a u32, b [10][20]f32 } }"
    ) == np.dtype((union, (10, 20)))

    assert ns.to_dtype("[10][20] f32") == np.dtype((np.float32, (10, 20)))

    # composes with the complex codes from before
    assert ns.to_dtype("struct { z cu16, w [4]cf64 }") == np.dtype(
        [("z", [("re", np.uint8), ("im", np.uint8)]), ("w", np.complex64, (4,))]
    )

    bad_ones = [
        "struct { }",                 # empty record
        "struct { i u8 b u32 }",      # missing comma
        "struct { i u8, i u32 }",     # duplicate field
        "struct { i u8",              # unclosed
        "[0]f32",                     # zero-length dim
        "[x]f32",                     # non-numeric dim
        "[10]",                       # no element type
        # "f32 junk",                   # trailing tokens
        "struct { 1a u8 }",           # bad field name
    ]

    for bad in bad_ones:
        with pytest.raises(ValueError):
            ns.to_dtype(bad)
