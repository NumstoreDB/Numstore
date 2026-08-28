import os
import sys
from pathlib import Path

import numpy as np
from setuptools import Extension, setup

HERE = Path(__file__).resolve().parent
SOURCES_FILE = HERE / "sources.txt"


def read_sources(path: Path) -> list[str]:
    if not path.exists():
        sys.exit(
            f"error: {path.name} not found.\n"
            "Run `make python-sources` from the repository root first."
        )

    lines = [
        line.strip()
        for line in path.read_text().splitlines()
        if line.strip() and "IGNORE" not in line
    ]

    if not lines:
        sys.exit(
            f"error: {path.name} is empty.\n"
            "Run `make python-sources` from the repository root first."
        )

    return lines


ext = Extension(
    name="pynumstore._pynumstore",
    sources=read_sources(SOURCES_FILE),
    include_dirs=["../../src", "./src/c", np.get_include()],
    define_macros=[
        ("NDEBUG", None),
        ("NLOG", None),
    ],
)

setup(
    name="pynumstore",
    packages=["pynumstore"],
    package_dir={"pynumstore": "src/pynumstore"},
    ext_modules=[ext],
    options={"build": {"build_base": os.environ.get("PYNUMSTORE_BUILD_BASE", "build")}},
)
