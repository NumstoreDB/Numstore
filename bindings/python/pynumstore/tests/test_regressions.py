"""Tests for numpy -> numstore conversion."""

import os
import pytest

import pynumstore as ns

def test_regression_txn_was_failing_on_close_didnt_exist():
    """
    transactions were always failing saying the weren't part of an existing 
    transaction on commit or rollback
    """

    ############## Success On a newly open database
    if os.path.isfile("mydb"):
        os.remove("mydb")

    with ns.open("mydb") as db:
        with db.begin_txn() as txn:
            var = txn.get_or_create("foo", "u32")
            assert var.name == "foo"

    ############## Success On an existing database
    with ns.open("mydb") as db:
        with db.begin_txn() as txn:
            var = txn.get_or_create("foo", "u32")
            assert var.name == "foo"

    ############## Failure On a newly open database
    if os.path.isfile("mydb"):
        os.remove("mydb")

    with pytest.raises(RuntimeError):
        with ns.open("mydb") as db:
            with db.begin_txn() as txn:
                var = txn.get_or_create("foo", "invalid")

    ############## Failure On an existing database
    with pytest.raises(RuntimeError):
        with ns.open("mydb") as db:
            with db.begin_txn() as txn:
                var = txn.get_or_create("foo", "invalid")
