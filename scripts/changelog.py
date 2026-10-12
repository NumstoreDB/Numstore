#!/usr/bin/env python3
# Print the most recent CHANGELOG.md entry.
lines = open("CHANGELOG.md").readlines()
headings = [i for i, line in enumerate(lines) if line.startswith("## [")]
end = headings[1] if len(headings) > 1 else len(lines)
print("".join(lines[headings[0] : end]).rstrip())
