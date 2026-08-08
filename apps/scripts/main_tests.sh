#!/usr/bin/env bash
mkdir -p build
python3 src/scripts/add_copywrite.py
python3 src/scripts/format_code.py
python3 src/scripts/gen_tests.py
python3 src/scripts/amalgamate.py
mv -f numstore.c build
pushd build

# Address 
while true; do
  gcc numstore.c -DTESTING -o numstore -g -O3 -fsanitize=address || break
  ./numstore --unit_tests 2>unit_tests_asan.log || break
  ./numstore --irwr foo 10 12314 2>irwr_asan.log || break
  ./numstore --cgd foo 2 12314 2>cgd_asan.log || break
  break
done

# Undefined 
while true; do
  gcc numstore.c -DTESTING -o numstore -g -O3 -fsanitize=undefined -fno-sanitize-recover=all || break
  ./numstore --unit_tests 2>unit_tests_undefined.log || break
  ./numstore --irwr foo 10 12314 2>irwr_undefined.log || break
  ./numstore --cgd foo 2 12314 2>cgd_undefined.log || break
  break
done

# Thread
while true; do
  gcc numstore.c -DTESTING -o numstore -g -O3 -fsanitize=thread || break
  ./numstore --unit_tests 2>unit_tests_thread.log || break
  ./numstore --irwr foo 10 12314 2>irwr_thread.log || break
  ./numstore --cgd foo 2 12314 2>cgd_thread.log || break
  break
done

# Leak
while true; do
  gcc numstore.c -DTESTING -o numstore -g -O3 -fsanitize=leak || break
  ./numstore --unit_tests 2>unit_tests_leak.log || break
  ./numstore --irwr foo 10 12314 2>irwr_leak.log || break
  ./numstore --cgd foo 2 12314 2>cgd_leak.log || break
  break
done

# Address and Undefined
while true; do
  gcc numstore.c -DTESTING -o numstore -g -O3 -fsanitize=address,undefined -fno-sanitize-recover=all || break
  ./numstore --unit_tests 2>unit_tests_asan_ubsan.log || break
  ./numstore --irwr foo 10 12314 2>irwr_asan_ubsan.log || break
  ./numstore --cgd foo 2 12314 2>cgd_asan_ubsan.log || break
  break
done

# Coverage
while true; do
  gcc numstore.c -DTESTING --coverage -fprofile-update=atomic -o numstore -g -O3 || break
  ./numstore --unit_tests 2>unit_tests_coverage.log || break
  ./numstore --irwr foo 10 12314 2>irwr_coverage.log || break
  ./numstore --cgd foo 2 12314 2>cgd_coverage.log || break
  gcov -b -c numstore.c >coverage_summary.txt 2>&1 || break
  break
done

# Valgrind
if ! command -v valgrind >/dev/null 2>&1; then
  echo "valgrind not installed, skipping"
else
  VG="valgrind \
    --tool=memcheck \
    --leak-check=full \
    --show-leak-kinds=all \
    --errors-for-leak-kinds=all \
    --track-origins=yes \
    --track-fds=yes \
    --trace-children=yes \
    --num-callers=50 \
    --read-var-info=yes \
    --expensive-definedness-checks=yes \
    --fair-sched=yes \
    --error-exitcode=1"
  while true; do
    gcc numstore.c -DTESTING -o numstore -g -O3 || break
    $VG --log-file=unit_tests_valgrind.log ./numstore --unit_tests || break
    $VG --log-file=irwr_valgrind.log       ./numstore --irwr foo 10 12314 || break
    $VG --log-file=cgd_valgrind.log        ./numstore --cgd foo 2 12314 || break
    break
  done
fi

# Static
while true; do
  gcc numstore.c -DTESTING -fanalyzer -O3 -c -o /dev/null 2>analyzer.log || break
  break
done

popd
