#!/usr/bin/env python3
import sys
import re
from pathlib import Path

def find_tests(root):
    pattern = re.compile(r'TEST\s*\(\s*(\w+)\s*\)')
    root = Path(root)

    if not root.exists():
        print(f"Error: Directory '{root}' does not exist.", file=sys.stderr)
        sys.exit(1)

    for path in sorted(root.rglob("*.c")):
        try:
            content = path.read_text(errors="replace")
        except Exception:
            continue

        matches = pattern.findall(content)
        if not matches:
            continue

        for t in matches:
            print(t)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: ./gen_tests.py <search_dir...>", file=sys.stderr)
        sys.exit(1)

    search_dirs = sys.argv[1:]

    for search_dir in search_dirs:
        find_tests(search_dir)
