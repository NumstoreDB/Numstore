#!/usr/bin/env bash
#
# Remove every build artifact in the tree.
#
# The top level dist/ is left alone on purpose: it holds the cross compiled
# release tarballs and wheels, which take a long time to reproduce.

set -eu
cd "$(dirname "$0")/.."

# Echo what a step is about to delete as a few wrapped lines, then delete it all
# in one go -- echoing or invoking rm per path is much slower.
list() {
  local what=$1 paths=$2
  if [ -z "$paths" ]; then
    echo "== $what: nothing to do"
    return 1
  fi
  echo "== $what ($(printf '%s\n' "$paths" | wc -l | tr -d ' ') paths)"
  printf '%s\n' "$paths" | tr '\n' ' ' | fold -s -w 96 \
    | awk '{ sub(/^ +/, ""); sub(/ +$/, ""); print "   " $0 }'
}

# Only name the fixed targets that actually exist.
existing() {
  local p
  for p in "$@"; do
    [ -e "$p" ] && printf '%s\n' "$p"
  done
  return 0
}

if list "removing build dirs" "$(
  existing build build_coverage nsserver/target wheelhouse \
           bindings/python/dist bindings/python/build bindings/python/sources.txt \
           docs/build docs/api
)"; then
  rm -rf build build_coverage nsserver/target wheelhouse \
         bindings/python/dist bindings/python/build bindings/python/sources.txt \
         docs/build docs/api
fi

caches=$(
  find . -type d \( -name __pycache__ -o -name '*.egg-info' -o -name .pytest_cache -o -name .ruff_cache \) -prune -print
)
if list "removing caches" "$caches"; then
  printf '%s\n' "$caches" | tr '\n' '\0' | xargs -0 rm -rf
fi

objs=$(
  find . -type f \( -name '*.o' -o -name '*.d' -o -name '*.gcda' -o -name '*.gcno' -o -name '*.gcov' \) -print
)
if list "removing object/coverage files" "$objs"; then
  printf '%s\n' "$objs" | tr '\n' '\0' | xargs -0 rm -f
fi

junk=$(
  find . -type f \( -name '*.db' -o -name '*.nsdb' -o -name '*.wal' -o -name 'testdb*' -o -name 'vgcore.*' -o -name gmon.out \) -print
)
if list "removing test databases and crash dumps" "$junk"; then
  printf '%s\n' "$junk" | tr '\n' '\0' | xargs -0 rm -f
fi
