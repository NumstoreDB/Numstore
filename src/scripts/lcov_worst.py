#!/usr/bin/env python3
"""
Sort an lcov .info report so the worst-covered (or least-called) functions
float to the top and the best-covered settle at the bottom.

The .info file is what `lcov -c -d ... -o file.info` produces (the same file
genhtml consumes). We parse it directly so this doesn't require lcov or
genhtml to be installed.

Per-function metrics we extract:
  - HITS:       how many times the function was called
                  (FNDA:<count>,<name>)
  - LINES:      executed lines / executable lines inside the function body
                  (DA:<line>,<count> filtered to the function's line range)
  - COVERAGE:   LINES.hit / LINES.total

Line ranges are estimated as [FN_line, next_FN_line - 1] in the same source
file, then capped at the last DA: line. lcov's basic format does not record
function ends, so this is approximate but matches what genhtml shows in
practice.

Usage:
    python lcov_worst.py coverage.info
    python lcov_worst.py coverage.info --sort hits         # least called first
    python lcov_worst.py coverage.info -n 25               # top 25 only
    python lcov_worst.py coverage.info --zero-only         # never-called only
    python lcov_worst.py coverage.info --filter src/rope   # path substring
"""

import argparse
import sys


def parse_lcov(path: str):
    """Yield one record per (file, function) found in an lcov .info file.

    Supports both lcov 1.x and lcov 2.x record formats:

      1.x:  FN:<line>,<name>           function declaration
            FNDA:<count>,<name>        call count

      2.x:  FNL:<idx>,<start>,<end>    function leader (exact line range)
            FNA:<idx>,<count>,<name>   alias; multiple FNA may share one FNL
                                       (inlines, C++ ctor aliases, etc.)

    Both can coexist in the same file. When 2.x records are present for a
    function we prefer them (the line range is exact, not estimated).
    """
    current_sf = None
    # lcov 1.x
    fn_decl = {}     # name -> declared line
    fn_hits = {}     # name -> call count
    # lcov 2.x
    fn_ranges = {}   # idx -> (start_line, end_line)
    fn_aliases = []  # list of (idx, count, name)
    # both
    line_hits = {}   # source line -> exec count

    def flush():
        if current_sf is None:
            return
        emitted = set()

        # Prefer lcov 2.x records when available.
        for idx, cnt, name in fn_aliases:
            rng = fn_ranges.get(idx)
            if rng is None:
                continue
            start_ln, end_ln = rng
            in_fn = [(ln, c) for ln, c in line_hits.items()
                     if start_ln <= ln <= end_ln]
            in_fn.sort()
            total = len(in_fn)
            hit = sum(1 for _, c in in_fn if c > 0)
            yield {
                "file":         current_sf,
                "name":         name,
                "line":         start_ln,
                "hits":         cnt,
                "lines_total":  total,
                "lines_hit":    hit,
                "coverage":     (hit / total * 100.0) if total else 0.0,
                "missed_lines": [ln for ln, c in in_fn if c == 0],
            }
            emitted.add(name)

        # 1.x fallback for anything not already emitted.
        sorted_fns = sorted(
            ((n, ln) for n, ln in fn_decl.items() if n not in emitted),
            key=lambda kv: kv[1],
        )
        max_line = max(line_hits) if line_hits else 0
        for i, (name, start_ln) in enumerate(sorted_fns):
            end_ln = (sorted_fns[i + 1][1] - 1
                      if i + 1 < len(sorted_fns) else max_line)
            in_fn = [(ln, c) for ln, c in line_hits.items()
                     if start_ln <= ln <= end_ln]
            in_fn.sort()
            total = len(in_fn)
            hit = sum(1 for _, c in in_fn if c > 0)
            yield {
                "file":         current_sf,
                "name":         name,
                "line":         start_ln,
                "hits":         fn_hits.get(name, 0),
                "lines_total":  total,
                "lines_hit":    hit,
                "coverage":     (hit / total * 100.0) if total else 0.0,
                "missed_lines": [ln for ln, c in in_fn if c == 0],
            }

    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.strip()
            if not line:
                continue

            if line.startswith("SF:"):
                current_sf = line[3:]
                fn_decl, fn_hits = {}, {}
                fn_ranges, fn_aliases = {}, []
                line_hits = {}

            elif line.startswith("FNL:"):
                # FNL:<idx>,<start>[,<end>]   (lcov 2.x)
                parts = line[4:].split(",")
                if len(parts) >= 2:
                    try:
                        idx = int(parts[0])
                        start = int(parts[1])
                        end = int(parts[2]) if len(parts) >= 3 else start
                        fn_ranges[idx] = (start, end)
                    except ValueError:
                        pass

            elif line.startswith("FNA:"):
                # FNA:<idx>,<count>,<name>    (lcov 2.x; name may contain ',')
                parts = line[4:].split(",", 2)
                if len(parts) >= 3:
                    try:
                        fn_aliases.append(
                            (int(parts[0]), int(parts[1]), parts[2]))
                    except ValueError:
                        pass

            elif line.startswith("FN:"):
                # FN:<line>,<name>            (lcov 1.x)
                # FN:<start>,<end>,<name>     (some 2.x emitters)
                parts = line[3:].split(",", 2)
                if len(parts) == 2:
                    ln, name = parts
                else:
                    ln, _, name = parts
                try:
                    fn_decl[name] = int(ln)
                except ValueError:
                    pass

            elif line.startswith("FNDA:"):
                # FNDA:<count>,<name>         (lcov 1.x)
                cnt, name = line[5:].split(",", 1)
                try:
                    fn_hits[name] = int(cnt)
                except ValueError:
                    pass

            elif line.startswith("DA:"):
                parts = line[3:].split(",")
                try:
                    line_hits[int(parts[0])] = int(parts[1])
                except (ValueError, IndexError):
                    pass

            elif line == "end_of_record":
                yield from flush()
                current_sf = None

        # Trailing record without explicit end_of_record (some emitters).
        if current_sf is not None:
            yield from flush()


def main():
    ap = argparse.ArgumentParser(
        description="Sort lcov .info by worst-covered functions first.")
    ap.add_argument("info_file", help="path to an lcov .info file")
    ap.add_argument("--sort", choices=["coverage", "hits"], default="coverage",
                    help="primary sort key (default: coverage, ascending). "
                         "'hits' surfaces never-called functions first.")
    ap.add_argument("-n", "--limit", type=int,
                    help="show only the top N worst rows")
    ap.add_argument("--zero-only", action="store_true",
                    help="show only functions that were never called")
    ap.add_argument("--no-uncov-lines", action="store_true",
                    help="hide functions whose body has zero instrumented "
                         "lines (e.g. pure declarations, inlines)")
    ap.add_argument("--filter", metavar="SUBSTR", default="",
                    help="only include rows where the file path contains "
                         "SUBSTR")
    ap.add_argument("--csv", action="store_true",
                    help="output CSV instead of a formatted table")
    args = ap.parse_args()

    try:
        records = list(parse_lcov(args.info_file))
    except OSError as e:
        print(f"could not read {args.info_file}: {e}", file=sys.stderr)
        sys.exit(2)

    if args.filter:
        records = [r for r in records if args.filter in r["file"]]
    if args.zero_only:
        records = [r for r in records if r["hits"] == 0]
    if args.no_uncov_lines:
        records = [r for r in records if r["lines_total"] > 0]

    # Worst first. Tiebreak so functions with 0 hits still sort above 50%-cov
    # ones with hits, and stable ordering by file:line for reproducibility.
    if args.sort == "coverage":
        records.sort(key=lambda r: (r["coverage"], r["hits"],
                                    r["file"], r["line"]))
    else:  # hits
        records.sort(key=lambda r: (r["hits"], r["coverage"],
                                    r["file"], r["line"]))

    if args.limit:
        records = records[:args.limit]

    if args.csv:
        import csv
        w = csv.writer(sys.stdout)
        w.writerow(["coverage_pct", "hits", "lines_hit", "lines_total",
                    "function", "file", "line"])
        for r in records:
            w.writerow([f"{r['coverage']:.1f}", r["hits"],
                        r["lines_hit"], r["lines_total"],
                        r["name"], r["file"], r["line"]])
        return

    if not records:
        # Sniff the file so the user can see WHY there's nothing.
        from collections import Counter
        kinds = Counter()
        try:
            with open(args.info_file, "r", encoding="utf-8",
                      errors="replace") as f:
                for raw in f:
                    s = raw.strip()
                    if not s:
                        continue
                    if s == "end_of_record":
                        kinds["end_of_record"] += 1
                    elif ":" in s:
                        kinds[s.split(":", 1)[0]] += 1
                    else:
                        kinds["(other)"] += 1
        except OSError:
            pass
        print("# no functions matched. record types seen in file:",
              file=sys.stderr)
        for k, v in kinds.most_common():
            print(f"#   {v:>8}  {k}", file=sys.stderr)
        if "FN" not in kinds:
            print("# the file has no FN: records, so per-function "
                  "coverage can't be computed.", file=sys.stderr)
            print("# this usually means lcov captured line data but not "
                  "function data —", file=sys.stderr)
            print("# common on macOS with llvm-cov. try:", file=sys.stderr)
            print("#   lcov --gcov-tool ~/bin/llvm-gcov.sh --rc "
                  "geninfo_unexecuted_blocks=1 \\", file=sys.stderr)
            print("#        --capture --directory . -o coverage.info",
                  file=sys.stderr)
        return

    name_w = max(len(r["name"]) for r in records)
    name_w = min(max(name_w, 8), 50)

    header = f"{'COV%':>6}  {'HITS':>8}  {'LINES':>9}  " \
             f"{'FUNCTION':<{name_w}}  FILE:LINE"
    print(header)
    print("-" * len(header))
    for r in records:
        lines_str = f"{r['lines_hit']}/{r['lines_total']}"
        name = r["name"]
        if len(name) > name_w:
            name = name[:name_w - 1] + "…"
        print(f"{r['coverage']:>5.1f}%  {r['hits']:>8}  {lines_str:>9}  "
              f"{name:<{name_w}}  {r['file']}:{r['line']}")


if __name__ == "__main__":
    main()
