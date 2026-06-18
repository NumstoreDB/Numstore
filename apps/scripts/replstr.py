import os

ROOTS = ["src", "include", "apps", "bindings"]
OLD, NEW = "page_h.h", "pager.h"

for ROOT in ROOTS:
    for dirpath, _, files in os.walk(ROOT):
        for name in files:
            path = os.path.join(dirpath, name)
            try:
                with open(path, "r", encoding="utf-8") as f:
                    text = f.read()
            except (UnicodeDecodeError, PermissionError):
                continue  # skip binaries / unreadable files
            if OLD in text:
                with open(path, "w", encoding="utf-8") as f:
                    f.write(text.replace(OLD, NEW))
                print(f"updated: {path}")
