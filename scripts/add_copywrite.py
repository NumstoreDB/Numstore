#!/usr/bin/env python3
"""Add or normalize the copyright header on C/H files."""

import os
import re
import argparse
from pathlib import Path

FILE_EXTENSIONS = (".c", ".h")
IGNORE_BASENAMES = {"testing.h", "assert.h"}
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

_LINE_COMMENT_RE = re.compile(r"^\s*//")
_COPYRIGHT_RE = re.compile(r"copyright|license|licenced|spdx", re.IGNORECASE)

RULE_NAME = "copyright"


def transform(path: Path, lines: list[str]) -> tuple[list[str], list[str], list[str]]:
    """Returns (new_lines, warnings, errors)."""
    start = 0
    while start < len(lines) and lines[start].strip() == "":
        start += 1

    end = start
    if end < len(lines):
        if lines[end].strip().startswith("/*"):
            while end < len(lines) and "*/" not in lines[end]:
                end += 1
            end += 1
        elif _LINE_COMMENT_RE.match(lines[end]):
            while end < len(lines) and _LINE_COMMENT_RE.match(lines[end]):
                end += 1

    existing = "".join(lines[start:end])
    rest = lines[end:] if (existing.strip() and _COPYRIGHT_RE.search(existing)) else lines[start:]
    new_lines = (COPYRIGHT_HEADER + "\n\n").splitlines(keepends=True) + rest
    return new_lines, [], []


def process_file(path: Path) -> bool:
    if path.name in IGNORE_BASENAMES:
        return False
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        lines = f.readlines()

    new_lines, warnings, errors = transform(path, lines)
    for w in warnings:
        print(f"WARN  [{RULE_NAME}] {path}: {w}")
    for e in errors:
        print(f"ERROR [{RULE_NAME}] {path}: {e}")
    if new_lines != lines:
        with open(path, "w", encoding="utf-8") as f:
            f.writelines(new_lines)
        print(f"MODIFIED [{RULE_NAME}] {path}")
    return bool(errors)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", nargs="?", default=".")
    args = parser.parse_args()

    had_errors = False
    for root, _, files in os.walk(args.directory):
        parts = Path(root).parts
        if any(p.startswith(".") or p in SKIP_DIRS for p in parts):
            continue
        for fname in sorted(files):
            if fname.endswith(FILE_EXTENSIONS):
                if process_file(Path(root) / fname):
                    had_errors = True

    if had_errors:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
