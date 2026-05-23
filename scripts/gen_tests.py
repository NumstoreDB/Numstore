#!/usr/bin/env python3
import sys
import re
from pathlib import Path

def find_tests(root, suite_name):
    pattern = re.compile(r'TEST\s*\(\s*(\w+)\s*\)')
    root = Path(root)
    
    if not root.exists():
        print(f"Error: Directory '{root}' does not exist.", file=sys.stderr)
        sys.exit(1)
        
    for path in sorted(root.rglob("*.c")):
        try:
            content = path.read_text(errors="replace")
        except Exception:
            continue
            
        matches = pattern.findall(content)
        if not matches:
            continue
            
        for t in matches:
            print(f"  REGISTER ({suite_name}, {t});")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: ./generate_test_calls.py <suite_name> <search_dir>", file=sys.stderr)
        sys.exit(1)
        
    suite = sys.argv[1]
    search_dir = sys.argv[2]
    
    find_tests(search_dir, suite)
