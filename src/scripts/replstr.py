import os

ROOTS = ["src", "src/testing", "src/templates", "src/samples", "src/testing", "src/tools"]

OLD, NEW = "i_printf", "i_log_printf"

for ROOT in ROOTS:
    for name in os.listdir(ROOT):
        path = os.path.join(ROOT, name)
        if not os.path.isfile(path):
            continue
        try:
            with open(path, "r", encoding="utf-8") as f:
                text = f.read()
        except (UnicodeDecodeError, PermissionError):
            continue
        if OLD in text:
            with open(path, "w", encoding="utf-8") as f:
                f.write(text.replace(OLD, NEW))
            print(f"updated: {path}")
