from __future__ import annotations

import pytest

import pynumstore as ns

@pytest.fixture
def db(tmp_path):
    database = ns.Database(str(tmp_path / "test.db"))
    yield database
    database.close()
