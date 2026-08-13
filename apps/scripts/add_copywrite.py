#!/usr/bin/env python3
import os
from pathlib import Path

# Paths are matched relative to the walk root (e.g. "testing_only/testing.h"),
# so this correctly skips that file no matter how deep the tree goes.
IGNORE_RELATIVE_PATHS = {"testing_only/testing.h", "csx_assert.h"}
SKIP_DIRS = {"build", "node_modules"}

COPYRIGHT_HEADER = """\
/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License."""


def add_copyright(root_dir: str = "."):
    root_dir = os.path.abspath(root_dir)
    modified = 0
    skipped = 0
    errors = 0

    for root, dirs, files in os.walk(root_dir):
        # Prune hidden and skipped directories in-place, recursively at every level
        dirs[:] = sorted(d for d in dirs if not d.startswith(".") and d not in SKIP_DIRS)

        for fname in sorted(files):
            if not fname.endswith((".c", ".h")):
                continue

            path = Path(root) / fname
            rel_path = path.relative_to(root_dir).as_posix()

            # Match against both the relative path and bare filename, so
            # "testing_only/testing.h" and a bare "csx_assert.h" both work
            # regardless of how deep they live in the tree.
            if rel_path in IGNORE_RELATIVE_PATHS or path.name in IGNORE_RELATIVE_PATHS:
                skipped += 1
                continue

            try:
                content = path.read_text(errors="replace")
            except OSError as e:
                print(f"ERROR reading {path}: {e}")
                errors += 1
                continue

            # If the file already has the license, leave it alone
            if "Licensed under the Apache License" in content:
                skipped += 1
                continue

            new_content = f"{COPYRIGHT_HEADER}\n\n{content}"

            try:
                path.write_text(new_content, encoding="utf-8")
            except OSError as e:
                print(f"ERROR writing {path}: {e}")
                errors += 1
                continue

            print(f"Modified {path.relative_to(root_dir)}")
            modified += 1

    print(f"\nDone. Modified {modified}, skipped {skipped}, errors {errors}.")


if __name__ == "__main__":
    add_copyright(".")
