from __future__ import annotations

import pytest

import pynumstore as ns


@pytest.fixture
def db_dir(tmp_path, monkeypatch):
    monkeypatch.chdir(tmp_path)
    return tmp_path


@pytest.fixture
def db(db_dir):
    database = ns.Database("t.db")
    yield database
    database.close()
