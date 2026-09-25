#!/usr/bin/env python3
import re

with open("CHANGELOG.md") as f:
    lines = f.readlines()

headings = [i for i, line in enumerate(lines) if re.match(r"^##\s+\[.+?\]", line)]

start = headings[0]
end = headings[1] if len(headings) > 1 else len(lines)

print("".join(lines[start:end]).rstrip())
