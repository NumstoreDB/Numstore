#!/usr/bin/env python3
from pathlib import Path

# I use this script a lot to replace
# tokens in the code base
# Not useful at all outside of that
# An ide would do a much better job
# I use it so much that I just keep it in
# the repo
REPLACEMENTS = {"SMF_END": "NS_END"}

for root in ("numstore/src", "numstore/include", "numstore/apps", "bindings", "docs"):
    for path in sorted(Path(root).rglob("*")):
        if not path.is_file():
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, PermissionError):
            continue

        new_text = text
        for old, new in REPLACEMENTS.items():
            new_text = new_text.replace(old, new)

        if new_text != text:
            path.write_text(new_text, encoding="utf-8")
            print(f"updated: {path}")
