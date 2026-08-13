#!/usr/bin/env python3
import os
from pathlib import Path
import re
import subprocess


def format_code():
    print("Checking and formatting files in 'src' and 'apps'...")
    print("-" * 50)

    # 2. Walk through specified directories
    for directory in ["src", "apps"]:
        if not os.path.exists(directory):
            continue

        for root, _, files in os.walk(directory):
            for file in files:
                if file.endswith((".c", ".h")):
                    file_path = os.path.join(root, file)

                    # 3. Check if the file needs formatting
                    # --output-replacements-xml is empty if the file matches the style guide
                    result = subprocess.run(
                            ["clang-format", "--style=file", "--output-replacements-xml", file_path],
                            capture_output=True,
                            text=True,
                            check=True
                            )

                    if "<replacement " in result.stdout:
                        print(f"Formatting: {file_path}")
                        # 4. Apply the formatting in-place
                        subprocess.run(["clang-format", "-i", "--style=file", file_path], check=True)

    print("-" * 50)
    print("Done!")

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

    template = Path("src/templates/unit_tests.c.in").read_text()

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

def amalgamate():
    # regex for a local include:  #include "foo.h" 
    LOCAL_INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
    SRC_DIR = "src"
    OUTPUT = "numstore.c"
    TEMPLATE_HEADERS = {"robin_hood_ht.h"}
    C_FILES = [
        "alloc.c",
        "error.c",
        "logging.c",
        "utils.c",
        "numerics.c",
        "serial.c",
        "types.c",
        "collections.c",
        "htable.c",
        "robin_hood_ht.c",
        "mem_vhmap.c",
        "compiler.c",
        "concurrency.c",
        "lock_table.c",
        "txn_table.c",
        "dirty_page_table.c",
        "page.c",
        "pager.c",
        "file_pager.c",
        "wal.c",
        "node_updates.c",
        "rope_algorithms.c",
        "var_algorithms.c",
        "variables.c",         
        "parsers.c",
        "query.c",
        "stride.c",
        "smartfiles.c",
        "os_common.c",
        "os_posix.c",
        "os_windows.c",
        "nsdb.c",
        "numstore.c",          
        "page_fixture.c",
        "smfile_test_fixture.c",
        "testing.c",
        "tests.c",
        "swarm_tests.c",
        "unit_tests.c",
    ]

    template_headers = { "robin_hood_ht.h" }
    emitted = set()   # normal headers already inlined (dedup)

    def banner(text):
        s = f"/************** {text} "
        return s + "*" * max(3, 78 - len(s)) + "/\n"


    def inline(name, out, stack):
        if name in stack:
            raise RuntimeError("circular include: " + " -> ".join(stack + [name]))
        path = os.path.join(SRC_DIR, name)
        out.write(banner(name))
        with open(path, encoding="utf-8", errors="replace") as f:
            for line in f:
                m = LOCAL_INCLUDE.match(line)
                if not m:
                    out.write(line)
                    continue
                hdr = m.group(1)

                if hdr in template_headers:
                    # paste body every time, no dedup
                    inline(hdr, out, stack + [name])
                elif os.path.exists(os.path.join(SRC_DIR, hdr)):
                    # normal local header: inline once, then drop
                    if hdr not in emitted:
                        emitted.add(hdr)
                        inline(hdr, out, stack + [name])
                    # else: already inlined -> drop the line
                else:
                    # unknown quoted header -> leave the #include line as-is
                    out.write(line)
        s = f"/************** {path} "
        out.write(banner(f"End {name}"))

    with open(OUTPUT, "w", encoding="utf-8") as out:
        out.write("/* Amalgamated numstore source. GENERATED - do not edit. */\n")
        out.write("#define NUMSTORE_AMALGAMATION 1\n\n")
        for c in C_FILES:
            inline(c, out, [])


if __name__ == '__main__':
    add_copywrite()
    gen_tests()
    format_code()
    amalgamate()
