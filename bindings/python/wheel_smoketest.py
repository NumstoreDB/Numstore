"""Post-build check that an installed pynumstore wheel actually works.

This is what cibuildwheel runs after building each wheel. It deliberately does
not use pytest and lives outside tests/ so that it stays independent of the
pytest suite: it exists to prove that the *compiled extension* loads and round
-trips data on the platform it was just built for, which is the part that
cross-compilation and per-platform compiler flags can break.

Run it against an installed wheel:

    python bindings/python/wheel_smoketest.py
"""

from __future__ import annotations

import sys
import tempfile
from pathlib import Path

import numpy as np

import pynumstore as ns


def check(label: str, got: object, want: object) -> None:
    if got != want:
        raise AssertionError(f"{label}: got {got!r}, want {want!r}")
    print(f"  ok  {label}")


def main() -> int:
    print(f"pynumstore smoke test on {sys.platform}, python {sys.version.split()[0]}")
    print(f"  extension: {ns._ns.__file__}")

    # dtype mapping - exercises the type table without touching the disk
    check("to_dtype('u32')", ns.to_dtype("u32"), np.dtype(np.uint32))
    check("to_dtype('f64')", ns.to_dtype("f64"), np.dtype(np.float64))

    with tempfile.TemporaryDirectory() as tmp:
        path = str(Path(tmp) / "smoke.db")

        with ns.Database(path) as db:
            db.execute("create foo u32")

            src = np.arange(5, dtype=np.uint32)
            check("write count", db.write(f"insert foo 0 {src.size}", src), src.size)

            got = db.read("read foo[0:]").ravel()
            if not np.array_equal(got, src):
                raise AssertionError(f"read back {got!r}, want {src!r}")
            print("  ok  read round-trip")

            # read into a preallocated buffer
            out = np.zeros(5, dtype=np.uint32)
            check("read_into count", db.read_into("read foo[0:]", out.reshape(5, 1)), 5)
            if not np.array_equal(out, src):
                raise AssertionError(f"read_into gave {out!r}, want {src!r}")
            print("  ok  read_into round-trip")

            # a committed transaction must be visible afterwards
            head = np.array([7, 8, 9], dtype=np.uint32)
            with db.begin() as txn:
                txn.write(f"insert foo 0 {head.size}", head)

            after = db.read("read foo[0:]").ravel()
            want = np.concatenate([head, src])
            if not np.array_equal(after, want):
                raise AssertionError(f"after commit {after!r}, want {want!r}")
            print("  ok  transaction commit visible")

            # a rolled back transaction must leave nothing behind
            with db.begin() as txn:
                txn.write("insert foo 0 2", np.array([42, 43], dtype=np.uint32))
                txn.rollback()

            after_rb = db.read("read foo[0:]").ravel()
            if not np.array_equal(after_rb, want):
                raise AssertionError(f"after rollback {after_rb!r}, want {want!r}")
            print("  ok  transaction rollback discarded")

            # variable metadata
            var = db.get("get foo")
            check("var name", var.name, "foo")
            check("var length", var.length, want.size)

    print("SMOKE TEST PASSED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
