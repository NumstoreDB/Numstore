# numstore (R)

R bindings for numstore, built with Rcpp on top of the C++ wrapper
(`src/numstore.hpp` / `src/numstore.cpp`).

## Install

You need R, the Rcpp package, a C++17 compiler, and the numstore C library
(`numstore.h` and `libnumstore`).

```sh
NUMSTORE_CFLAGS="-I/path/to/numstore/include" \
NUMSTORE_LIBS="-L/path/to/numstore/lib -Wl,-rpath,/path/to/numstore/lib" \
R CMD INSTALL numstoreR
```

Both variables can be left unset if numstore is on the default compiler and
linker paths. If you change any `// [[Rcpp::export]]` function in
`src/bindings.cpp`, regenerate the glue with `Rcpp::compileAttributes("numstoreR")`.

## Usage

```r
library(numstore)
db <- ns_open("test.db")

ns_transaction(db, function(tx) {
  ns_execute(tx, "create foo u32")
  ns_write(tx, "insert foo 0 10", 0:9, type = "u32")
})

tx <- ns_begin(db)
ns_read(tx, "read foo[0:10]", type = "u32")
ns_commit(tx)
ns_close(db)
```

See `inst/examples/example.R` for plans and smart files.

## Notes

- **Types.** Every read/write takes a numstore element type. `i8`, `u8`,
  `i16`, `u16` and `i32` come back as integer vectors; `u32`, `i64`, `u64`,
  `f32` and `f64` come back as doubles. 64-bit integers are exact only up to
  2^53. On write, values are range-checked, and NA/fractional values are
  rejected for integer types. An `i32` value of -2^31 reads back as `NA`,
  because that is R's integer NA.
- **Transactions are the context.** Query functions take the transaction,
  not the database; it already knows which database it belongs to.
- **Lifetimes.** Handles are freed by the garbage collector in any order.
  Closing a database (explicitly or via GC) rolls back its open transactions
  and frees its plans first. Any later use of them is a clean R error.
  `ns_crash()` drops open transactions without rolling them back.
- Handles can't be saved with `save()`/`saveRDS()` and reloaded.
