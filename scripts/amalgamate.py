#!/usr/bin/env python3
import os
import re
import sys
from pathlib import Path

LOCAL_INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
INCLUDE_DIRS = ["src"]           # mirrors your build's -I flags
TEMPLATE_HEADERS = {"ns_robin_hood_ht.h"}  # pasted every time, no dedup

emitted = set()  # paths already written to output


def banner(text):
    s = f"/************** {text} "
    return s + "*" * max(3, 78 - len(s)) + "/\n"


def resolve_include(hdr, current_dir):
    """current file's own dir first, then each INCLUDE_DIRS entry."""
    for base in (current_dir, *INCLUDE_DIRS):
        candidate = os.path.normpath(os.path.join(base, hdr))
        if os.path.exists(candidate):
            return candidate
    return None  # not found -- leave the #include line as-is


def strip_guard(text):
    """Remove a #ifndef/#define/#endif guard from template-header text.
    A guard breaks the 're-paste this fresh every time' pattern: once the
    guard macro is defined by the first paste, the preprocessor silently
    skips every later paste's body."""
    lines = text.splitlines(keepends=True)
    nb = [(i, l) for i, l in enumerate(lines) if l.strip()]
    if len(nb) < 3:
        return text
    (i0, l0), (i1, l1) = nb[0], nb[1]
    i_last, l_last = nb[-1]
    m_ifndef = re.match(r'^\s*#\s*ifndef\s+(\w+)\s*$', l0)
    m_define = re.match(r'^\s*#\s*define\s+(\w+)\s*$', l1)
    m_endif = re.match(r'^\s*#\s*endif\b', l_last)
    if not (m_ifndef and m_define and m_endif) or m_ifndef.group(1) != m_define.group(1):
        return text
    return "".join(l for idx, l in enumerate(lines) if idx not in (i0, i1, i_last))


def inline(path, out, stack):
    if path in stack:
        raise RuntimeError("circular include: " + " -> ".join(stack + [path]))
    directory = os.path.dirname(path)
    is_template = os.path.basename(path) in TEMPLATE_HEADERS
    content = Path(path).read_text(encoding="utf-8", errors="replace")
    if is_template:
        content = strip_guard(content)
    out.write(banner(path))
    for line in content.splitlines(keepends=True):
        m = LOCAL_INCLUDE.match(line)
        if not m:
            out.write(line)
            continue
        hdr_path = resolve_include(m.group(1), directory)
        if hdr_path is None:
            out.write(line)  # unresolved -- leave as-is
        elif os.path.basename(hdr_path) in TEMPLATE_HEADERS:
            inline(hdr_path, out, stack + [path])  # no dedup
        elif hdr_path not in emitted:
            emitted.add(hdr_path)
            inline(hdr_path, out, stack + [path])
        # else already emitted -> drop the line
    out.write(banner(f"End {path}"))


def amalgamate(sources_file):
    files = [f for f in Path(sources_file).read_text().split() if f]
    if not files:
        raise RuntimeError(f"{sources_file} was empty -- check ALL_SRCS in the Makefile")
    out = sys.stdout
    for path in files:
        if path in emitted:
            continue
        emitted.add(path)
        inline(path, out, [])


if __name__ == '__main__':
    amalgamate(sys.argv[1])
