#!/usr/bin/env python3
"""Collapse consecutive blank lines into a single blank line."""

import os
import argparse
from pathlib import Path

FILE_EXTENSIONS = (".c", ".h")
IGNORE_BASENAMES = {"testing.h", "assert.h"}
SKIP_DIRS = {"build", "node_modules"}

RULE_NAME = "collapse_blank_lines"


def transform(path: Path, lines: list[str]) -> tuple[list[str], list[str], list[str]]:
    new_lines = []
    prev_blank = False
    for line in lines:
        is_blank = line.strip() == ""
        if is_blank and prev_blank:
            continue
        new_lines.append(line)
        prev_blank = is_blank
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
