#!/usr/bin/env python3
"""
Scrape C source files for every function the user defines.

Walks a directory tree (or list of files), parses .c and .h files, and
prints every function *definition* it finds. Catches:
  - regular functions:           int foo(int x) { ... }
  - static / inline / extern:    static inline void bar(void) { ... }
  - multi-line declarators
  - __attribute__((...)) suffixes
  - return types that are pointers / multi-word (const char *, struct foo *, ...)

It ignores:
  - declarations / prototypes that end in `;`
  - calls and uses inside other functions
  - comments and string/char literals
  - preprocessor lines

Usage:
    python scrape_c_functions.py [PATH ...] [-v] [--exclude DIR]

If no path is given, the current directory is scanned.
"""

import argparse
import os
import re
import sys

# Things that look like `name(` but aren't function definitions.
CONTROL_KEYWORDS = {
    "if", "else", "while", "for", "switch", "do", "return",
    "sizeof", "typeof", "__typeof__", "alignof", "_Alignof",
    "__alignof__", "__alignof", "_Generic", "_Static_assert",
    "static_assert", "asm", "__asm", "__asm__", "defined",
    "case", "break", "continue", "goto", "va_arg", "offsetof",
    "register", "auto",
    # Compiler attribute keywords that can appear as `KW((...)) {` next to a
    # definition and would otherwise be mistaken for the function name.
    "__attribute__", "__declspec", "_Noreturn", "_Alignas", "_Atomic",
    "_Thread_local",
}


def strip_comments_and_strings(code: str) -> str:
    """Blank out comments and string/char literals so `{`, `(`, etc.
    inside them don't confuse the parser. Newlines are preserved so error
    messages referencing line numbers (if any) still make sense."""
    out = []
    i, n = 0, len(code)
    while i < n:
        c = code[i]
        # /* ... */
        if c == "/" and i + 1 < n and code[i + 1] == "*":
            j = code.find("*/", i + 2)
            if j == -1:
                break
            out.append(re.sub(r"[^\n]", " ", code[i:j + 2]))
            i = j + 2
            continue
        # // ...
        if c == "/" and i + 1 < n and code[i + 1] == "/":
            j = code.find("\n", i + 2)
            if j == -1:
                j = n
            out.append(" " * (j - i))
            i = j
            continue
        # "..."
        if c == '"':
            j = i + 1
            while j < n and code[j] != '"':
                j += 2 if code[j] == "\\" and j + 1 < n else 1
            out.append('""')
            i = j + 1
            continue
        # '...'
        if c == "'":
            j = i + 1
            while j < n and code[j] != "'":
                j += 2 if code[j] == "\\" and j + 1 < n else 1
            out.append("''")
            i = j + 1
            continue
        out.append(c)
        i += 1
    return "".join(out)


def strip_preprocessor(code: str) -> str:
    """Blank out preprocessor lines, including ones continued with `\\`."""
    out, in_pp = [], False
    for line in code.split("\n"):
        if in_pp or line.lstrip().startswith("#"):
            out.append("")
            in_pp = line.rstrip().endswith("\\")
        else:
            out.append(line)
    return "\n".join(out)


def find_matching(code: str, open_idx: int) -> int:
    """Given an index pointing at `(`, return the index of the matching `)`,
    or -1 if it isn't found."""
    depth, i, n = 0, open_idx, len(code)
    while i < n:
        ch = code[i]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def skip_post_paren(code: str, i: int) -> int:
    """Skip whitespace and qualifier-ish things that may appear between a
    parameter list and the function body: __attribute__((...)), asm("..."),
    const/volatile/restrict, etc."""
    n = len(code)
    while i < n:
        while i < n and code[i].isspace():
            i += 1
        if i >= n:
            return i
        if code.startswith("__attribute__", i):
            j = i + len("__attribute__")
            while j < n and code[j].isspace():
                j += 1
            if j < n and code[j] == "(":
                end = find_matching(code, j)
                if end == -1:
                    return n
                i = end + 1
                continue
            return i
        m = re.match(r"(?:__asm__|__asm|asm)\s*\(", code[i:])
        if m:
            j = i + m.end() - 1
            end = find_matching(code, j)
            if end == -1:
                return n
            i = end + 1
            continue
        m = re.match(r"(?:const|volatile|restrict|__restrict|__restrict__)\b",
                     code[i:])
        if m:
            i += m.end()
            continue
        return i
    return i


_IDENT_RE = re.compile(r"\b([a-zA-Z_]\w*)\s*\(")


def find_functions_in_text(code: str) -> set:
    """Return the set of function names defined in `code`."""
    code = strip_comments_and_strings(code)
    code = strip_preprocessor(code)

    found = set()
    n = len(code)

    for m in _IDENT_RE.finditer(code):
        name = m.group(1)
        if name in CONTROL_KEYWORDS:
            continue

        paren_open = m.end() - 1
        paren_close = find_matching(code, paren_open)
        if paren_close == -1:
            continue

        after = skip_post_paren(code, paren_close + 1)
        if after < n and code[after] == "{":
            found.add(name)

    return found


def iter_source_files(roots, exclude_dirs):
    for root in roots:
        if os.path.isfile(root):
            yield root
            continue
        for dirpath, dirnames, files in os.walk(root):
            # prune excluded dirs in-place so os.walk skips them
            dirnames[:] = [d for d in dirnames if d not in exclude_dirs]
            for f in files:
                if f.endswith((".c", ".h")):
                    yield os.path.join(dirpath, f)


def main():
    ap = argparse.ArgumentParser(
        description="List unique function definitions in C source files.")
    ap.add_argument("paths", nargs="*", default=["."],
                    help="files or directories to scan (default: .)")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="also show which file each function was found in")
    ap.add_argument("--exclude", action="append", default=[],
                    help="directory name to skip (repeatable); "
                         "e.g. --exclude build --exclude third_party")
    args = ap.parse_args()

    exclude = set(args.exclude)
    all_functions = set()
    by_file = {}

    for path in iter_source_files(args.paths, exclude):
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                code = f.read()
        except OSError as e:
            print(f"# could not read {path}: {e}", file=sys.stderr)
            continue
        names = find_functions_in_text(code)
        if names:
            by_file[path] = names
            all_functions.update(names)

    if args.verbose:
        for path in sorted(by_file):
            for name in sorted(by_file[path]):
                print(f"{path}: {name}")
        print(f"# {len(all_functions)} unique functions across "
              f"{len(by_file)} files", file=sys.stderr)
    else:
        for name in sorted(all_functions):
            print(name)


if __name__ == "__main__":
    main()
