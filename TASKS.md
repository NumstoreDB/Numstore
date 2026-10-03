Publish pynumstore to pypi and post an announcement on reddit
=============================================================
- [ ] Cross compile numstore on major platforms - use cibuildwheel to compile 
- [ ] A set of pynumstore tests
- [ ] All memory leaks via valgrind for ns_simtest and unit_tests are fixed
- [ ] Design a basic locking mechanic for multiple processes
    - Lock the entire database for a process - but internally a process has
      hierarchial locking
    - Probably fine to do X and S
- [ ] Docs
- [ ] Basic github actions with code coverage reported on the main repo for
  numstore library
- [ ] Fix broken seeds 
    - seed=53529417 seqid=7642841
      "$exe" \
        --dbname "simtest_macos-latest.db" \
        --duration 10 \
        --seed "$seed" \
        --commit-hash "ab8b955c1f890852b1a9d5f21e81d5366de73e18" \
        --seqid "$seqid"

Supported Architectures
=======================
- linux-x86_64
- linux-aarch64
- macos-arm64
- macos-x86_64
- windows-x86_64
