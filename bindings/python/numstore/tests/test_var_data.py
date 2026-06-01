"""
Basic insert, read, remove, and write tests
"""

import pytest
import os
import numpy as np
import numstore as ns

def test_insert_read_remove_write_autotxn(subtests):
    if os.path.isfile("mydb"):
        os.remove("mydb")

    with ns.open("mydb") as db:

        ############## Setup
        with subtests.test("create variable"):
            db.get_or_create("y", "f32")

        ############## Insert (append)
        with subtests.test("append data"):
            var = db.get("y")
            var.append(np.array([1.0, 2.0, 3.0], dtype=np.float32))
            var.append(np.array([4.0, 5.0, 6.0], dtype=np.float32))

        ############## Read
        with subtests.test("read full array"):
            var = db.get("y")
            result = var[0:]
            assert len(result) == 6
            np.testing.assert_array_equal(result, [1.0, 2.0, 3.0, 4.0, 5.0, 6.0])

        ############## Insert in middle
        with subtests.test("insert in middle"):
            var = db.get("y")
            var.insert(2, np.array([7.0, 8.0], dtype=np.float32))
            result = var[0:]
            assert len(result) == 8
            np.testing.assert_array_equal(result, [1.0, 2.0, 7.0, 8.0, 3.0, 4.0, 5.0, 6.0])

        ############## Write (overwrite slice)
        with subtests.test("overwrite slice"):
            var = db.get("y")
            var[1:4] = np.array([9.0, 9.0, 9.0], dtype=np.float32)
            result = var[0:]
            np.testing.assert_array_equal(result, [1.0, 9.0, 9.0, 9.0, 3.0, 4.0, 5.0, 6.0])

        ############## Remove slice
        with subtests.test("remove every other element"):
            var = db.get("y")
            del var[0::2]
            result = var[0:]
            np.testing.assert_array_equal(result, [9.0, 9.0, 4.0, 6.0])

        ############## Remove remaining
        with subtests.test("remove all remaining"):
            var = db.get("y")
            del var[0:]
            result = var[0:]
            assert len(result) == 0


def test_insert_read_remove_write_txn(subtests):
    if os.path.isfile("mydb"):
        os.remove("mydb")

    with ns.open("mydb") as db:

        ############## Setup
        with subtests.test("create variable"):
            with db.begin_txn() as txn:
                txn.get_or_create("y", "f32")

        ############## Insert (append)
        with subtests.test("append data"):
            with db.begin_txn() as txn:
                var = txn.get("y")
                var.append(np.array([1.0, 2.0, 3.0], dtype=np.float32))
                var.append(np.array([4.0, 5.0, 6.0], dtype=np.float32))

        ############## Read
        with subtests.test("read full array"):
            with db.begin_txn() as txn:
                var = txn.get("y")
                result = var[0:]
                assert len(result) == 6
                np.testing.assert_array_equal(result, [1.0, 2.0, 3.0, 4.0, 5.0, 6.0])

        ############## Insert in middle
        with subtests.test("insert in middle"):
            with db.begin_txn() as txn:
                var = txn.get("y")
                var.insert(2, np.array([7.0, 8.0], dtype=np.float32))
            with db.begin_txn() as txn:
                var = txn.get("y")
                result = var[0:]
                assert len(result) == 8
                np.testing.assert_array_equal(result, [1.0, 2.0, 7.0, 8.0, 3.0, 4.0, 5.0, 6.0])

        ############## Write (overwrite slice)
        with subtests.test("overwrite slice"):
            with db.begin_txn() as txn:
                var = txn.get("y")
                var[1:4] = np.array([9.0, 9.0, 9.0], dtype=np.float32)
            with db.begin_txn() as txn:
                var = txn.get("y")
                result = var[0:]
                np.testing.assert_array_equal(result, [1.0, 9.0, 9.0, 9.0, 3.0, 4.0, 5.0, 6.0])

        ############## Rolled-back write does not persist
        with subtests.test("rolled back write does not persist"):
            with pytest.raises(RuntimeError):
                with db.begin_txn() as txn:
                    var = txn.get("y")
                    var[0:4] = np.array([0.0, 0.0, 0.0, 0.0], dtype=np.float32)
                    raise RuntimeError("force rollback")
            with db.begin_txn() as txn:
                var = txn.get("y")
                result = var[0:]
                np.testing.assert_array_equal(result, [1.0, 9.0, 9.0, 9.0, 3.0, 4.0, 5.0, 6.0])

        ############## Remove slice
        with subtests.test("remove every other element"):
            with db.begin_txn() as txn:
                var = txn.get("y")
                del var[0::2]
            with db.begin_txn() as txn:
                var = txn.get("y")
                result = var[0:]
                np.testing.assert_array_equal(result, [9.0, 9.0, 4.0, 6.0])

        ############## Remove remaining
        with subtests.test("remove all remaining"):
            with db.begin_txn() as txn:
                var = txn.get("y")
                del var[0:]
            with db.begin_txn() as txn:
                var = txn.get("y")
                result = var[0:]
                assert len(result) == 0
