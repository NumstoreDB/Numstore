"""
Basic get and create and delete tests
"""

import pytest
import os

import pynumstore as ns

def test_get_create_var_autotxn(subtests):
    if os.path.isfile("mydb"):
        os.remove("mydb")

    with ns.open("mydb") as db:

        ############## Throw an error if variable doesn't exist
        with subtests.test("get non-existent variable raises"):
            with pytest.raises(RuntimeError):
                db.get("foo")

        ############## Throw an error for invalid types
        with subtests.test("get_or_create with invalid type raises"):
            with pytest.raises(RuntimeError):
                db.get_or_create("foo", "32")
            with pytest.raises(RuntimeError):
                db.get_or_create("foo", "invalid type")

        ############## Success
        with subtests.test("get_or_create success"):
            db.get_or_create("foo", "u32")

        ############## Throw an error for duplicate variable
        with subtests.test("create duplicate variable raises"):
            with pytest.raises(RuntimeError):
                db.create("foo", "u32")

        ############## Get Success
        with subtests.test("get existing variable success"):
            var = db.get("foo")
            assert var.name == "foo"

        ############## Delete a non existant variable
        with subtests.test("delete non existing variable"):
            with pytest.raises(RuntimeError):
                db.delete("bar")

        ############## Delete success
        with subtests.test("delete an existing variable"):
            db.delete("foo")

        ############## Throw an error if variable was deleted
        with subtests.test("get deleted variable raises"):
            with pytest.raises(RuntimeError):
                db.get("foo")

        ############## Create after delete success
        with subtests.test("get_or_create success"):
            db.get_or_create("foo", "u32")

        ############## Delete one more time for good measure
        with subtests.test("delete a variable again"):
            db.delete("foo")

def test_get_create_var_txn(subtests):
    if os.path.isfile("mydb"):
        os.remove("mydb")

    with ns.open("mydb") as db:

        ############## Throw an error if variable doesn't exist
        with subtests.test("get non-existent variable raises"):
            with pytest.raises(RuntimeError):
                with db.begin_txn() as txn:
                    txn.get("foo")

        ############## Throw an error for invalid type
        with subtests.test("get_or_create with invalid type raises"):
            with pytest.raises(RuntimeError):
                with db.begin_txn() as txn:
                    txn.get_or_create("foo", "32")

        ############## Success Rollback Failure
        with subtests.test("error mid-txn rolls back all changes"):
            with pytest.raises(RuntimeError):
                with db.begin_txn() as txn:
                    # Success
                    txn.get_or_create("foo", "u32") 

                    # Success 
                    var = txn.get("foo")
                    assert var.name == "foo"

                    # Failed - invalid type
                    txn.get_or_create("bar", "invalid") 

                    # should rollback

            # Failed (no variable)
            with db.begin_txn() as txn:
                with pytest.raises(RuntimeError):
                    txn.get("bar")

        ############## Success
        with subtests.test("get_or_create success"):
            with db.begin_txn() as txn:
                txn.get_or_create("foo", "u32")

        ############## Throw an error for duplicate variable
        with subtests.test("create duplicate variable raises"):
            with pytest.raises(RuntimeError):
                with db.begin_txn() as txn:
                    txn.create("foo", "u32")

        ############## Success
        with subtests.test("get existing variable success"):
            with db.begin_txn() as txn:
                var = txn.get("foo")
                assert var.name == "foo"

        ############## Delete a non existant variable
        with subtests.test("delete non existing variable raises"):
            with pytest.raises(RuntimeError):
                with db.begin_txn() as txn:
                    txn.delete("bar")

        ############## Delete success
        with subtests.test("delete an existing variable"):
            with db.begin_txn() as txn:
                txn.delete("foo")

        ############## Throw an error if variable was deleted
        with subtests.test("get deleted variable raises"):
            with pytest.raises(RuntimeError):
                with db.begin_txn() as txn:
                    txn.get("foo")

        ############## Create after delete success
        with subtests.test("get_or_create after delete success"):
            with db.begin_txn() as txn:
                txn.get_or_create("foo", "u32")

        ############## Rollback a delete - variable should still exist
        with subtests.test("rolled back delete does not persist"):
            with pytest.raises(RuntimeError):
                with db.begin_txn() as txn:
                    txn.delete("foo")
                    raise RuntimeError("force rollback")
            with db.begin_txn() as txn:
                var = txn.get("foo")
                assert var.name == "foo"

        ############## Delete one more time for good measure
        with subtests.test("delete a variable again"):
            with db.begin_txn() as txn:
                txn.delete("foo")
