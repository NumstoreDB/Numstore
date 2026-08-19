#!/usr/bin/env python3

import os
from pathlib import Path
import re
import subprocess


def gen_tests():
    pattern = re.compile(r'TEST\s*\(\s*(\w+)\s*\)')

    tests = []  # list of (name, filepath, line_number)
    for root in ["src"]:
        root = Path(root)
        if not root.exists():
            print(f"Error: Directory '{root}' does not exist.", file=sys.stderr)
            sys.exit(1)
        for path in sorted(root.rglob("*.c")):
            try:
                content = path.read_text(errors="replace")
            except Exception:
                continue
            for m in pattern.finditer(content):
                line = content.count("\n", 0, m.start()) + 1
                tests.append((m.group(1), path, line))

    template = Path("src/unit_tests.c.in").read_text()

    calls = ""
    for name, filepath, line in tests:
        calls += f"""\

  if (!filter || strstr("{name}", filter))
  {{
    extern void __test__{name}(void);
    i_log_info("========================= TEST CASE: %s\\n", "{name}");
    int prev = test_ret;
    test_ret = 0;
    __test__{name}();
    if (!test_ret)
    {{
      i_log_passed("%s\\n", "{name}");
      test_ret = prev;
    }}
    else
    {{
      failed_names[failed++] = "{name}";
    }}
    ntests++;
  }}
"""

    out = template.replace("%CALLS%", calls)
    out = out.replace("%TEST_COUNT%", str(len(tests)))
    Path("src/unit_tests.c").write_text(out)
    print(f"Generated src/unit_tests.c with {len(tests)} tests.")

gen_tests()
