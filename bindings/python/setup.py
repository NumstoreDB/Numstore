from __future__ import annotations

import shutil
import subprocess
import sysconfig
from pathlib import Path

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

REPO_ROOT = Path(__file__).resolve().parents[2]


class MakeBuildExt(build_ext):
    """Delegates the actual C compilation to the repo's top-level Makefile."""

    def build_extension(self, ext: Extension) -> None:
        subprocess.check_call(["make", "python"], cwd=REPO_ROOT)

        soabi = sysconfig.get_config_var("EXT_SUFFIX")
        built = REPO_ROOT / "build" / "python" / "target" / f"_pynumstore{soabi}"
        if not built.exists():
            raise FileNotFoundError(f"expected build output at {built}")

        dest = Path(self.get_ext_fullpath(ext.name))
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(built, dest)


setup(
    ext_modules=[Extension("pynumstore._pynumstore", sources=[])],
    cmdclass={"build_ext": MakeBuildExt},
)
