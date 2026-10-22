from __future__ import annotations

from importlib.metadata import version
from pathlib import Path

import pynumstore as ns

VERSION_TXT = Path(__file__).resolve().parent.parent / "version.txt"


def test_wheel_version_matches_version_txt():
    assert version("pynumstore") == VERSION_TXT.read_text().strip()


def test_package_exposes_dunder_version():
    assert ns.__version__ == VERSION_TXT.read_text().strip()
