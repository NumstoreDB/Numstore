import os
import pytest
from hypothesis import settings, HealthCheck

settings.register_profile(
    "default",
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow],
    max_examples=50,
)
settings.load_profile("default")

import pynumstore as ns

DB_PATH = "hypothesis_test.db"

@pytest.fixture(scope="session")
def db():
    if os.path.isfile(DB_PATH):
        os.remove(DB_PATH)
    with ns.open(DB_PATH) as database:
        yield database

@pytest.fixture(scope="session", autouse=True)
def _set_session_db(db):
    import test_data_pbt as m
    m._session_db = db
