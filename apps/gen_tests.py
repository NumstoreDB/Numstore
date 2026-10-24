#!/usr/bin/env python3

import re
import sys
from pathlib import Path

OS_BACKENDS = ["posix", "windows"]


def gen_tests(template_path, output_path, platform):
    pattern = re.compile(r"TEST\s*\(\s*(\w+)\s*\)")

    skip = [Path("csrc/core/os") / b for b in OS_BACKENDS if b != platform]

    tests = []
    for root in ["csrc"]:
        root = Path(root)
        if not root.exists():
            print(f"Error: Directory '{root}' does not exist.", file=sys.stderr)
            sys.exit(1)
        for path in sorted(root.rglob("*.c")):
            if any(d in path.parents for d in skip):
                continue
            try:
                content = path.read_text(errors="replace")
            except Exception:
                continue
            for m in pattern.finditer(content):
                line = content.count("\n", 0, m.start()) + 1
                tests.append((m.group(1), path, line))

    template = Path(template_path).read_text()

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
    Path(output_path).write_text(out)
    print(f"Generated {output_path} with {len(tests)} tests.")


if len(sys.argv) != 4 or sys.argv[3] not in OS_BACKENDS:
    print(
        f"Usage: {sys.argv[0]} TEMPLATE OUTPUT ({'|'.join(OS_BACKENDS)})",
        file=sys.stderr,
    )
    sys.exit(1)

gen_tests(sys.argv[1], sys.argv[2], sys.argv[3])
