#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed or not in PATH." >&2
    exit 1
fi

echo "Running clang-format on .c and .h files in 'src' and 'apps'..."

# Find and format files
# -i alters the files in-place
# --style=file tells it to look for a .clang-format file in the project
find src apps -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i --style=file {} +

echo "Formatting complete!"
