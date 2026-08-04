Release Build Instructions
==========================

To build numstore, choose your favorite compiler and compile the 
amalgamated numstore.c file.

You can compile numstore.c as a library to embedd into your project 
Or you can compile numstore.c as a cli repl by adding the `-DNUMSTORE_EXE` compiler flag

The following macros can be defined to change code behavior:

- `-DNDEBUG`       - Remove assertions - this should be added for anything release related 
- `-DNLOG`         - Remove any logging - this should be added for anything release related 
- `-DTESTING`      - Add tests in the final library or executable (you'll also want NUMSTORE_EXE)
- `-DNUMSTORE_EXE` - Compiles the numstore executable (a cli repl for interacting with the db)

The preferred debug format is:

`gcc numstore.c smfile_sample1_basic_crud.c -o sample -g`

The preferred release format is:

`gcc numstore.c smfile_sample1_basic_crud.c -o sample -DNDEBUG -DNLOG -O3`

Or choose your favorite compiler
