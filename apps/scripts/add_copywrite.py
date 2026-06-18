#!/usr/bin/env python3
import os
from pathlib import Path

IGNORE_BASENAMES = {"testing/testing.h", "csx_assert.h"}
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

def process_file(path: Path):
    if path.name in IGNORE_BASENAMES:
        return

    content = path.read_text(errors="replace")

    # If the file already has the license, do absolutely nothing
    if "Licensed under the Apache License" in content:
        return

    # Prepend header and update file
    path.write_text(f"{COPYRIGHT_HEADER}\n\n{content}", encoding="utf-8")
    print(f"Modified {path}")

if __name__ == '__main__':
    # Walk the directory tree smoothly using pathlib
    for root, dirs, files in os.walk("."):
        # Prune hidden or skipped directories out of the walk in-place
        dirs[:] = [d for d in dirs if not d.startswith(".") and d not in SKIP_DIRS]

        for fname in sorted(files):
            if fname.endswith((".c", ".h")):
                process_file(Path(root) / fname)
