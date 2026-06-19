#!/usr/bin/env python3

import sys
import re
from pathlib import Path

def find_tests(roots):
    pattern = re.compile(r'TEST\s*\(\s*(\w+)\s*\)')
    tests = []  # list of (name, filepath, line_number)
    for root in roots:
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
    return tests

def make_calls(tests):
    out = ""
    for name, filepath, line in tests:
        out += f"""\

  //////////////////// {filepath}:{line} START
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
  //////////////////// {filepath}:{line} DONE
"""
    return out

def generate(tests, template_path, output_path):
    template = Path(template_path).read_text()
    out = template.replace("%CALLS%", make_calls(tests))
    out = out.replace("%TEST_COUNT%", str(len(tests)))
    Path(output_path).write_text(out)
    print(f"Generated {output_path} with {len(tests)} tests.")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: gen_tests.py <template_file> <output_file> <search_dir...>", file=sys.stderr)
        sys.exit(1)
    template_path = sys.argv[1]
    output_path   = sys.argv[2]
    search_dirs   = sys.argv[3:]
    tests = find_tests(search_dirs)
    if not tests:
        print("Error: No tests found.", file=sys.stderr)
        sys.exit(1)
    generate(tests, template_path, output_path)
