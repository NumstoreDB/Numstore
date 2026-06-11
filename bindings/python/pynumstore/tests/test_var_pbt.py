"""
Hypothesis property tests for pynumstore.

A single DB is opened once for the entire session. Each test deletes any
names it owns at the start of the test body so that Hypothesis replays of
failing examples start from a clean slate without needing to recreate the DB.
"""

import os
import pytest
from hypothesis import given, assume, settings, HealthCheck
import hypothesis.strategies as st
from hypothesis.stateful import RuleBasedStateMachine, rule, invariant

import pynumstore as ns

# ---------------------------------------------------------------------------
# Strategies
# ---------------------------------------------------------------------------

VALID_TYPES = st.sampled_from(["u32", "u64", "i32", "i64", "f32", "f64"])

INVALID_TYPES = st.one_of(
    st.just("32"),
    st.just("invalid"),
    st.just("U32"),
    st.text().filter(
        lambda s: s not in {"u32", "u64", "i32", "i64", "f32", "f64"} and len(s) > 0
    ),
)

VALID_NAMES = st.text(
    alphabet=st.characters(whitelist_categories=("Lu", "Ll", "Nd"), whitelist_characters="_"),
    min_size=1,
    max_size=64,
)

DISTINCT_NAME_LISTS = st.lists(VALID_NAMES, min_size=2, max_size=8, unique=True)

# ---------------------------------------------------------------------------
# Session-scoped DB fixture
# ---------------------------------------------------------------------------

DB_PATH = "hypothesis_test.db"


@pytest.fixture(scope="session")
def db():
    if os.path.isfile(DB_PATH):
        os.remove(DB_PATH)
    with ns.open(DB_PATH) as database:
        yield database


# ---------------------------------------------------------------------------
# Helper
# ---------------------------------------------------------------------------

def delete_if_exists(db, *names):
    """Delete names that may have been left over from a prior Hypothesis example."""
    for name in names:
        try:
            db.delete(name)
        except RuntimeError:
            pass


# ---------------------------------------------------------------------------
# Class wrapper so @given tests can receive the session-scoped db fixture
# ---------------------------------------------------------------------------

class TestProperties:

    @pytest.fixture(autouse=True)
    def _db(self, db):
        self.db = db

    @given(name=VALID_NAMES, typ=VALID_TYPES)
    def test_name_roundtrip_autotxn(self, name, typ):
        # Delete to clean up
        delete_if_exists(self.db, name)

        # Get or create
        self.db.get_or_create(name, typ)

        # Get 
        var = self.db.get(name)

        # Assert same
        assert var.name == name

        # Delete it
        self.db.delete(name)

    @given(name=VALID_NAMES, typ=VALID_TYPES)
    def test_name_roundtrip_txn(self, name, typ):
        # Delete to clean up
        delete_if_exists(self.db, name)
        with self.db.begin_txn() as txn:
            # Get or create
            txn.get_or_create(name, typ)

            # Get
            var = txn.get(name)

            # Assert same
            assert var.name == name

        # Delete it
        delete_if_exists(self.db, name)

    @given(name=VALID_NAMES, typ=INVALID_TYPES)
    def test_invalid_type_always_raises_autotxn(self, name, typ):
        # Delete to clean up
        delete_if_exists(self.db, name)

        with pytest.raises(RuntimeError):
            # Get or create - invalid type
            self.db.get_or_create(name, typ)

    @given(name=VALID_NAMES, typ=INVALID_TYPES)
    def test_invalid_type_always_raises_txn(self, name, typ):
        # Delete to clean up
        delete_if_exists(self.db, name)

        with pytest.raises(RuntimeError):
            with self.db.begin_txn() as txn:

                # Get or create - invalid type
                txn.get_or_create(name, typ)

    @given(name=VALID_NAMES, typ=VALID_TYPES)
    def test_get_or_create_idempotent_autotxn(self, name, typ):
        # Delete to clean up
        delete_if_exists(self.db, name)

        # Get
        self.db.get_or_create(name, typ)

        # Get - must not raise
        self.db.get_or_create(name, typ)   

        # Same name
        assert self.db.get(name).name == name

        # Delete
        delete_if_exists(self.db, name)

    @given(name=VALID_NAMES, typ=VALID_TYPES)
    def test_get_or_create_idempotent_txn(self, name, typ):
        # Delete to clean up
        delete_if_exists(self.db, name)
        with self.db.begin_txn() as txn:

            # Get
            txn.get_or_create(name, typ)

            # Get - must not raise
            txn.get_or_create(name, typ)   

            # Same name
            assert txn.get(name).name == name

        # Delete it
        delete_if_exists(self.db, name)

    # -----------------------------------------------------------------------
    # 4. create duplicate always raises
    # -----------------------------------------------------------------------

    @given(name=VALID_NAMES, typ=VALID_TYPES)
    def test_create_duplicate_raises_autotxn(self, name, typ):
        delete_if_exists(self.db, name)
        self.db.get_or_create(name, typ)
        with pytest.raises(RuntimeError):
            self.db.create(name, typ)
        delete_if_exists(self.db, name)

    @given(name=VALID_NAMES, typ=VALID_TYPES)
    def test_create_duplicate_raises_txn(self, name, typ):
        delete_if_exists(self.db, name)
        with self.db.begin_txn() as txn:
            txn.get_or_create(name, typ)
        with pytest.raises(RuntimeError):
            with self.db.begin_txn() as txn:
                txn.create(name, typ)
        delete_if_exists(self.db, name)

    # -----------------------------------------------------------------------
    # 5. get on non-existent name always raises
    # -----------------------------------------------------------------------

    @given(name=VALID_NAMES)
    def test_get_nonexistent_raises_autotxn(self, name):
        delete_if_exists(self.db, name)
        with pytest.raises(RuntimeError):
            self.db.get(name)

    @given(name=VALID_NAMES)
    def test_get_nonexistent_raises_txn(self, name):
        delete_if_exists(self.db, name)
        with pytest.raises(RuntimeError):
            with self.db.begin_txn() as txn:
                txn.get(name)

    # -----------------------------------------------------------------------
    # 6. delete then get always raises
    # -----------------------------------------------------------------------

    @given(name=VALID_NAMES, typ=VALID_TYPES)
    def test_get_after_delete_raises_autotxn(self, name, typ):
        delete_if_exists(self.db, name)
        self.db.get_or_create(name, typ)
        self.db.delete(name)
        with pytest.raises(RuntimeError):
            self.db.get(name)

    @given(name=VALID_NAMES, typ=VALID_TYPES)
    def test_get_after_delete_raises_txn(self, name, typ):
        delete_if_exists(self.db, name)
        with self.db.begin_txn() as txn:
            txn.get_or_create(name, typ)
        with self.db.begin_txn() as txn:
            txn.delete(name)
        with pytest.raises(RuntimeError):
            with self.db.begin_txn() as txn:
                txn.get(name)

    # -----------------------------------------------------------------------
    # 7. Deleting one variable does not affect others
    # -----------------------------------------------------------------------

    @given(names=DISTINCT_NAME_LISTS, typ=VALID_TYPES)
    def test_delete_one_does_not_affect_others(self, names, typ):
        for name in names:
            delete_if_exists(self.db, name)
        for name in names:
            self.db.get_or_create(name, typ)

        target, *survivors = names
        self.db.delete(target)

        with pytest.raises(RuntimeError):
            self.db.get(target)
        for name in survivors:
            assert self.db.get(name).name == name

        for name in survivors:
            delete_if_exists(self.db, name)

    # -----------------------------------------------------------------------
    # 8. Transaction atomicity: exception rolls back all writes
    # -----------------------------------------------------------------------

    @given(name=VALID_NAMES, typ=VALID_TYPES)
    def test_txn_rollback_on_exception(self, name, typ):
        delete_if_exists(self.db, name)
        with pytest.raises(RuntimeError):
            with self.db.begin_txn() as txn:
                txn.get_or_create(name, typ)
                raise RuntimeError("forced rollback")
        with pytest.raises(RuntimeError):
            with self.db.begin_txn() as txn:
                txn.get(name)

    @given(name=VALID_NAMES, typ=VALID_TYPES)
    def test_txn_rollback_preserves_delete(self, name, typ):
        delete_if_exists(self.db, name)
        with self.db.begin_txn() as txn:
            txn.get_or_create(name, typ)
        with pytest.raises(RuntimeError):
            with self.db.begin_txn() as txn:
                txn.delete(name)
                raise RuntimeError("forced rollback")
        with self.db.begin_txn() as txn:
            assert txn.get(name).name == name
        delete_if_exists(self.db, name)

    @given(names=DISTINCT_NAME_LISTS, typ=VALID_TYPES)
    def test_txn_rollback_of_multiple_creates(self, names, typ):
        for name in names:
            delete_if_exists(self.db, name)
        with pytest.raises(RuntimeError):
            with self.db.begin_txn() as txn:
                for name in names:
                    txn.get_or_create(name, typ)
                raise RuntimeError("forced rollback")
        for name in names:
            with pytest.raises(RuntimeError):
                with self.db.begin_txn() as txn:
                    txn.get(name)
