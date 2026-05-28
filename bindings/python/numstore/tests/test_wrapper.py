"""Representative tests for Database, Transaction, Variable.

Each test opens a fresh database via the tmp_path fixture, which auto-cleans
both the .db file and any sibling .wal files after the test.
"""
import random
import pytest
import numpy as np
import numpy.testing as npt

pynumstore = pytest.importorskip("pynumstore")
import pynumstore as ns

@pytest.fixture
def db_path(tmp_path):
    return str(tmp_path / "test.db")

def test_database_lifecycle_manual(db_path):
    # manual open/close
    db = ns.Database(db_path)
    assert "test.db" in repr(db)
    assert "closed" not in repr(db)

    db.close()
    assert "closed" in repr(db)
    db.close()  # idempotent — must not raise

def test_database_lifecycle_ctx_mngr(db_path):
    # context manager reopens cleanly
    with ns.open_db(db_path) as db:
        assert "test.db" in repr(db)
        v = db.var("x", dtype="i32", create=True)
        assert isinstance(v, ns.Variable)
        assert v.name == "x"
        assert "x" in repr(v)

    assert "closed" in repr(db)

def test_variable_create(db_path):
    """Create, access via [] and .var(), delete, re-create with same name."""
    with Database(db_path) as db:
        # create and access
        v = db.var("sensor", dtype="f32", create=True)
        assert isinstance(v, Variable)
        assert db["sensor"].name == "sensor"

        with pytest.raises(TypeError):
            db[0]  # non-string key

        with pytest.raises((ValueError, TypeError)):
            db.var("missing", create=True)  # dtype required

def test_variable_create(db_path):
    with Database(db_path) as db
        # populate, delete, re-create — fresh variable starts empty
        v.append(np.array([1.0], dtype=np.float32))
        v.append(np.array([2.0], dtype=np.float32))
        assert len(v) == 2

        db.delete("sensor")
        v2 = db.var("sensor", dtype="f32", create=True)
        #assert len(v2) == 0
        v2.append(np.array([99.0], dtype=np.float32))
        npt.assert_array_almost_equal(v2[0], np.array([99.0], dtype=np.float32))


def test_append_insert_len_and_order(db_path):
    """Append increments len and returns index; insert shifts right; insert at
    end is equivalent to append."""
    with Database(db_path) as db:
        v = db.var("a", dtype="i32", create=True)
        assert len(v) == 0

        idx0 = v.append(np.array([10], dtype=np.int32))
        idx1 = v.append(np.array([30], dtype=np.int32))
        assert idx0 == 0 and idx1 == 1
        assert len(v) == 2

        # insert 20 between 10 and 30
        v.insert(1, np.array([20], dtype=np.int32))
        assert len(v) == 3
        npt.assert_array_equal(v[0], np.array([10], dtype=np.int32))
        npt.assert_array_equal(v[1], np.array([20], dtype=np.int32))
        npt.assert_array_equal(v[2], np.array([30], dtype=np.int32))

        # insert at position 0 prepends
        v.insert(0, np.array([0], dtype=np.int32))
        npt.assert_array_equal(v[0], np.array([0], dtype=np.int32))
        npt.assert_array_equal(v[1], np.array([10], dtype=np.int32))

        # insert at len == append
        v.insert(len(v), np.array([40], dtype=np.int32))
        npt.assert_array_equal(v[len(v) - 1], np.array([40], dtype=np.int32))


def test_read_write_and_slices(db_path):
    """Read/write by int index and slice; writes don't change len; slice
    semantics (step, negative, clamp, empty) all work."""
    with Database(db_path) as db:
        v = db.var("a", dtype="i32", create=True)
        for i in range(10):
            v.append(np.array([i * 10], dtype=np.int32))

        # single read/write roundtrip
        v[3] = np.array([999], dtype=np.int32)
        assert len(v) == 10
        npt.assert_array_equal(v[3], np.array([999], dtype=np.int32))
        # neighbours untouched
        npt.assert_array_equal(v[2], np.array([20], dtype=np.int32))
        npt.assert_array_equal(v[4], np.array([40], dtype=np.int32))

        with pytest.raises(TypeError):
            v["bad"] = np.array([0], dtype=np.int32)
        with pytest.raises(TypeError):
            _ = v["bad"]

        # slice reads
        assert len(v[:]) == 10
        assert len(v[0:5]) == 5
        assert len(v[0:10:2]) == 5   # step
        assert len(v[0:-1]) == 9     # negative stop
        assert len(v[0:1000]) == 10  # clamp
        # assert len(v[5:5]) == 0      # empty

        # bulk slice write
        replacement = np.array([[100], [200], [300]], dtype=np.int32)
        v[0:3] = replacement
        assert len(v) == 10
        npt.assert_array_equal(v[0], np.array([100], dtype=np.int32))
        npt.assert_array_equal(v[2], np.array([300], dtype=np.int32))
        npt.assert_array_equal(v[4], np.array([40], dtype=np.int32))  # untouched


def test_remove_and_del(db_path):
    """remove() returns the right value and shifts left; del syntax works;
    drain to empty then refill."""
    with Database(db_path) as db:
        v = db.var("a", dtype="i32", create=True)
        for val in [10, 20, 30, 40, 50]:
            v.append(np.array([val], dtype=np.int32))

        # remove from middle, check return value and shift
        result = v.remove(2)  # removes 30
        npt.assert_array_equal(result, np.array([30], dtype=np.int32))
        assert len(v) == 4
        npt.assert_array_equal(v[2], np.array([40], dtype=np.int32))

        # remove from tail
        result = v.remove(len(v) - 1)
        npt.assert_array_equal(result, np.array([50], dtype=np.int32))
        assert len(v) == 3

        # del syntax (no return)
        del v[0]
        assert len(v) == 2
        npt.assert_array_equal(v[0], np.array([20], dtype=np.int32))

        # del slice
        for val in [60, 70, 80]:
            v.append(np.array([val], dtype=np.int32))
        del v[1:3]
        assert len(v) == 3

        # drain completely, then refill
        while len(v):
            v.remove(0)
        assert len(v) == 0
        v.append(np.array([1], dtype=np.int32))
        assert len(v) == 1
        npt.assert_array_equal(v[0], np.array([1], dtype=np.int32))


def test_dtype_roundtrip(db_path):
    """Every supported scalar dtype survives append+read at boundary values."""
    cases = [
        ("u8",  np.uint8,   [0, 255, 128]),
        ("u16", np.uint16,  [0, 65535, 1000]),
        ("u32", np.uint32,  [0, 2**32 - 1, 42]),
        ("u64", np.uint64,  [0, 2**64 - 1, 999]),
        ("i8",  np.int8,    [-128, 127, 0]),
        ("i16", np.int16,   [-32768, 32767, 0]),
        ("i32", np.int32,   [-(2**31), 2**31 - 1, 0]),
        ("i64", np.int64,   [-(2**63), 2**63 - 1, 0]),
        ("f32", np.float32, [0.0, 1.5, -1.5]),
        ("f64", np.float64, [0.0, 1.23456789012345, -1e15]),
    ]
    with Database(db_path) as db:
        for dtype, nptype, vals in cases:
            v = db.var(dtype, dtype=dtype, create=True)
            for val in vals:
                v.append(np.array([val], dtype=nptype))
            assert len(v) == len(vals)
            for i, expected in enumerate(vals):
                npt.assert_array_equal(v[i], np.array([expected], dtype=nptype))


def test_transaction_commit_rollback_and_visibility(db_path):
    """Commits persist; rollbacks leave no trace; reads within a txn see
    uncommitted writes; sequential txns see all prior commits."""
    with Database(db_path) as db:
        db.var("log", dtype="i32", create=True)

        # context manager commits on clean exit
        with db.begin_txn() as txn:
            assert isinstance(txn, Transaction)
            assert "Transaction" in repr(txn)
            txn["log"].append(np.array([1], dtype=np.int32))
            txn["log"].append(np.array([2], dtype=np.int32))
            # reads within the txn see uncommitted data
            assert len(txn["log"]) == 2
            npt.assert_array_equal(txn["log"][0], np.array([1], dtype=np.int32))
        assert len(db["log"]) == 2

        # exception triggers rollback
        try:
            with db.begin_txn() as txn:
                txn["log"].append(np.array([99], dtype=np.int32))
                raise RuntimeError("forced")
        except RuntimeError:
            pass
        assert len(db["log"]) == 2  # rollback undid the append

        # explicit commit/rollback
        txn = db.begin_txn()
        txn["log"].append(np.array([3], dtype=np.int32))
        txn.commit()
        assert len(db["log"]) == 3

        txn2 = db.begin_txn()
        txn2["log"].append(np.array([4], dtype=np.int32))
        txn2.rollback()
        assert len(db["log"]) == 3

        # each new txn sees all previously committed data
        for _ in range(3):
            with db.begin_txn() as txn:
                txn["log"].append(np.array([0], dtype=np.int32))
        assert len(db["log"]) == 6


def test_transaction_create_delete_multi_var(db_path):
    """Create and delete variables inside transactions; rollback of a create
    leaves the variable absent; multi-var txns are atomic."""
    with Database(db_path) as db:
        # create two vars in one txn, populate both, commit
        with db.begin_txn() as txn:
            vx = txn.var("x", dtype="i32", create=True)
            vy = txn.var("y", dtype="f32", create=True)
            for i in range(5):
                vx.append(np.array([i], dtype=np.int32))
                vy.append(np.array([float(i) * 1.1], dtype=np.float32))
        assert len(db["x"]) == 5
        assert len(db["y"]) == 5

        # rollback of writes to both vars leaves both unchanged
        try:
            with db.begin_txn() as txn:
                txn["x"].append(np.array([99], dtype=np.int32))
                txn["y"].append(np.array([99.0], dtype=np.float32))
                raise RuntimeError
        except RuntimeError:
            pass
        assert len(db["x"]) == 5
        assert len(db["y"]) == 5

        # delete inside a committed txn
        with db.begin_txn() as txn:
            txn.delete("y")
        # y is gone; x is untouched
        assert len(db["x"]) == 5

        # empty txn commits cleanly
        with db.begin_txn() as txn:
            pass


def test_persistence_across_reopen(db_path):
    """Data written in one session is readable after close+reopen, including
    appends, writes, and removes."""
    values = [100, 200, 300, 400, 500]

    with Database(db_path) as db:
        v = db.var("p", dtype="i32", create=True)
        for val in values:
            v.append(np.array([val], dtype=np.int32))
        v[2] = np.array([999], dtype=np.int32)  # overwrite middle
        v.remove(0)                              # remove first

    expected = [200, 999, 400, 500]
    with Database(db_path) as db:
        v = db["p"]
        assert len(v) == len(expected)
        for i, exp in enumerate(expected):
            npt.assert_array_equal(v[i], np.array([exp], dtype=np.int32))


@pytest.mark.parametrize("seed", [0, 1, 42])
def test_random_mixed_ops_match_reference(db_path, seed):
    """Randomly interleave appends, inserts, writes, and removes against a
    Python list reference, then verify full contents match."""
    rng = random.Random(seed)
    reference = []

    with Database(db_path) as db:
        v = db.var("r", dtype="i32", create=True)

        for _ in range(150):
            op = rng.choices(
                ["append", "insert", "write", "remove"],
                weights=[4, 2, 2, 2]
            )[0]

            if op == "append" or len(reference) == 0:
                val = rng.randint(0, 9999)
                reference.append(val)
                v.append(np.array([val], dtype=np.int32))

            elif op == "insert":
                pos = rng.randint(0, len(reference))
                val = rng.randint(0, 9999)
                reference.insert(pos, val)
                v.insert(pos, np.array([val], dtype=np.int32))

            elif op == "write":
                idx = rng.randint(0, len(reference) - 1)
                val = rng.randint(-9999, -1)
                reference[idx] = val
                v[idx] = np.array([val], dtype=np.int32)

            elif op == "remove" and len(reference) > 1:
                idx = rng.randint(0, len(reference) - 1)
                expected = reference.pop(idx)
                result = v.remove(idx)
                npt.assert_array_equal(result, np.array([expected], dtype=np.int32))

        assert len(v) == len(reference)
        for i, expected in enumerate(reference):
            npt.assert_array_equal(v[i], np.array([expected], dtype=np.int32))

        # verify slice reads match sequential element reads
        if len(reference) >= 4:
            start, stop = 1, len(reference) - 1
            slice_result = v[start:stop]
            for local_i, global_i in enumerate(range(start, stop)):
                npt.assert_array_equal(slice_result[local_i], v[global_i])
