import os

ROOT = "libs"  
OLD, NEW = "->error_trace (e)", "->e.cause_code "

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
