#!/usr/bin/env bash
#
# Upload wheels to TestPyPI, or to PyPI with --prod.
#
#   scripts/upload_python.sh wheelhouse/*.whl
#   scripts/upload_python.sh --prod dist/wheels/*.whl
#
# Credentials come from twine: ~/.pypirc, or TWINE_USERNAME=__token__ plus
# TWINE_PASSWORD=<api token>.

set -eu
cd "$(dirname "$0")/.."

if [ "${1:-}" = "--prod" ]; then
  shift
  repo_args=""
  repo_name="PyPI"
else
  repo_args="--repository testpypi"
  repo_name="TestPyPI"
fi

echo "== uploading $# wheel(s) to $repo_name"
printf '%s\n' "$@" | fold -s -w 96 | sed 's/^/   /'

twine check "$@"
twine upload $repo_args "$@"
