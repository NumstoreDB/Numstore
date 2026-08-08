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

popd
