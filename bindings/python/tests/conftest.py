from __future__ import annotations

import pytest

import pynumstore as ns


@pytest.fixture
def db_dir(tmp_path, monkeypatch):
    """Run the test inside its own directory, so db names can be relative.

    numstore caps a database path at NS_NAME_MAX - 4 characters: 196 on unix,
    but only 46 on Windows, which is shorter than the absolute tmp_path pytest
    hands out (`C:\\Users\\runneradmin\\AppData\\Local\\Temp\\` alone is 39).
    A relative name resolved against the cwd is a few characters on every
    platform, and each test still gets its own directory.
    """
    monkeypatch.chdir(tmp_path)
    return tmp_path


@pytest.fixture
def db(db_dir):
    database = ns.Database("t.db")
    yield database
    database.close()
