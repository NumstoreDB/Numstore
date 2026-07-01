import os

ROOTS = ["src", "src/testing", "src/templates", "src/samples/numstore", "src/samples/smartfiles", "src/testing", "src/tools"]

OLD, NEW = "UNREACHABLE ();", "UNREACHABLE (); // LCOV_EXCL_LINE"

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
