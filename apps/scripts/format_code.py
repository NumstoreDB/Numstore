#!/usr/bin/env python3
import os
import subprocess
import sys
import shutil

def main():
    # 1. Check if clang-format is installed
    if not shutil.which("clang-format"):
        print("Error: clang-format is not installed or not in PATH.", file=sys.stderr)
        sys.exit(1)

    search_dirs = ["src", "apps"]
    extensions = (".c", ".h")

    print("Checking and formatting files in 'src' and 'apps'...")
    print("-" * 50)

    # 2. Walk through specified directories
    for directory in search_dirs:
        if not os.path.exists(directory):
            continue

        for root, _, files in os.walk(directory):
            for file in files:
                if file.endswith(extensions):
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

if __name__ == "__main__":
    main()
