Numstore
========

**A database for arrays**

Numstore is a single file embedded ACID database built for arrays written entirely
in C with no dependencies.

Conceptually, it's an ACID file with [faster inner file mutations](https://theolincke.com/blog/13_inner_inserts)

Getting Started 
===============

To get started, pick a platform and follow the steps:

<details>
<summary><strong>Linux / MacOS</strong></summary>

1. Amalgamte numstore into one source file:
    
        python3 src/scripts/amalgamate.py 

2. Compile the executable (by adding the NUMSTORE_EXE macro)

        gcc -o numstore -DNUMSTORE_EXE numstore.c
        ./numstore foo.db

3. You can remove all those debug logs and make it release worthy:

        # Hint - you can get rid of all that logging with:
        gcc -o numstore -O3 -DNDEBUG -DNUMSTORE_EXE -DNLOG numstore.c
        ./numstore foo.db

4. You can use numstore as a library by removing NUMSTORE_EXE flag:

        # Build sample apps by adding NUMSTORE_LIB
        gcc -o sample numstore.c src/samples/ns_sample1_basic_crud.c -Isrc -DNLOG -DNEBUG -O3
        ./sample

</details>

<details>
<summary><strong>Windows (MSVC)</strong></summary>

Run these from a Developer Command Prompt for VS (or after running
vcvars64.bat), so that `cl` is on your PATH.

1. Amalgamate numstore into one source file:

       python src\scripts\amalgamate.py

2. Compile the executable (by adding the NUMSTORE_EXE macro):

       cl /Fe:numstore.exe /DNUMSTORE_EXE numstore.c
       numstore.exe foo.db

3. Remove all those debug logs and make it release worthy:

       cl /Fe:numstore.exe /O2 /DNDEBUG /DNUMSTORE_EXE /DNLOG numstore.c
       numstore.exe foo.db

4. Use numstore as a library by removing the NUMSTORE_EXE flag
   (build sample apps by adding NUMSTORE_LIB):

       cl /Fe:sample.exe numstore.c src\samples\ns_sample1_basic_crud.c /Isrc /DNLOG /DNDEBUG /O2
       sample.exe

</details>

For more information, refer to the [Documentation](docs/index.md).

AI Usage Policy
===============

I use AI the way I use a language server: as a tool, not a co-author. AI usage is fine
but not for heavy tasks.

Things I ask AI to do:

- Add edge-case test scenarios to existing unit tests (reviewed before committing).
- Review an algorithm I've written and flag anything that looks wrong.
- Write formatting scripts, CI/CD glue, and other boilerplate I could write myself
  but would rather not.

Things I don't ask AI to do:

- Implement features.
- Delete or replace code I've written.
- Read a paper and implement the algorithm.

In practice, AI is useful for ideation, code review, and generating mundane code I'll
immediately refactor. Every algorithm in this codebase was written by me.

Contributing
============

File a ticket on GitHub for bugs, feature requests, or questions.
Many tickets that are easy to contribute will be marked

License
=======

Apache 2.0. See LICENSE.
