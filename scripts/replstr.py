import os

ROOTS = ["src"]

# Map of OLD -> NEW replacements
REPLACEMENTS = {
        "struct txn" : "struct ns_txn",
}

for ROOT in ROOTS:
    for dirpath, _, filenames in os.walk(ROOT):
        for name in filenames:
            path = os.path.join(dirpath, name)
            try:
                with open(path, "r", encoding="utf-8") as f:
                    text = f.read()
            except (UnicodeDecodeError, PermissionError):
                continue

            new_text = text
            hits = []
            for old, new in REPLACEMENTS.items():
                if old in new_text:
                    hits.append(old)
                    new_text = new_text.replace(old, new)

            if hits:
                with open(path, "w", encoding="utf-8") as f:
                    f.write(new_text)
                print(f"updated: {path} ({', '.join(hits)})")
