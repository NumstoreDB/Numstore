Numstore
========

Numstore is a single-file, embedded, ACID database for numerical arrays,
written in C with no dependencies. This archive contains the `numstore` CLI,
the `numstore`/`smartfiles` C libraries, headers, documentation, man pages,
and sample code - everything needed to link against Numstore or use it from
the command line.

Layout
------

```
.
├── bin/                    precompiled tools and sample binaries
├── include/
│   ├── numstore/           numstore.h  - database for numerical arrays
│   └── smartfiles/         smartfiles.h - plain ACID transactional file
├── lib/
│   ├── libnumstore.a       static library (numstore + smartfiles, one archive)
│   └── pkgconfig/
│       ├── numstore.pc
│       └── smartfiles.pc
└── share/
    ├── doc/numstore/
    │   ├── html/           browsable documentation (open html/index.html)
    │   └── markdown/       the same docs as plain markdown
    ├── man/
    │   ├── man1/           numstore(1), and other CLI tool pages
    │   └── man3/           library function pages, e.g. ns_var_get(3)
    └── numstore/examples/
        ├── Makefile        builds every sample below
        └── *.c             sample source (ns_*.c, smfile_*.c)
```

### `bin/`

| Binary                                | Purpose                            |
|---------------------------------------|------------------------------------|
| `numstore`                            | Main CLI / REPL (work in progress) |
| `nspprint`                            | Inspect a numstore page            |
| `nsspprint`                           | Inspect a numstore super-page      |
| `dlread`                              | Read a data-list page              |
| `walpprint`                           | Inspect a WAL record               |
| `print_query`                         | Parse and print a query AST        |
| `print_type`                          | Parse and print a type expression  |
| `resolve_type_ref`                    | Resolve a type reference           |
| `ns_sample1_basic_crud`               | Compiled numstore sample           |
| `smfile_sample1_basic_crud`           | Compiled smartfiles sample         |
| `smfile_sample2_transactions`         | Compiled smartfiles sample         |
| `smfile_sample3_stride`               | Compiled smartfiles sample         |
| `smfile_sample4_rollback_commit`      | Compiled smartfiles sample         |

Compiling Against the Library
------------------------------

### Manually

```
$ gcc samples/ns_sample1_basic_crud.c -I include -L lib -lnumstore \
    -o ns_sample1_basic_crud
```

`smartfiles.h` works the same way - link against the same `libnumstore.a`:

```
$ gcc samples/smfile_sample1_basic_crud.c -I include -L lib -lnumstore \
    -o smfile_sample1_basic_crud
```

### With pkg-config

```
$ export PKG_CONFIG_PATH="$PWD/lib/pkgconfig:$PKG_CONFIG_PATH"
$ gcc samples/ns_sample1_basic_crud.c $(pkg-config --cflags --libs numstore) \
    -o ns_sample1_basic_crud
```

The `.pc` files locate themselves automatically, so no extra flags are
needed regardless of where you extract this archive. If `pkg-config` reports
an unresolved variable, your installed pkg-config is too old for this - pass
the prefix explicitly instead:

```
$ pkg-config --define-variable=prefix=$PWD --cflags --libs numstore
```

### With the bundled sample Makefile

```
$ cd share/numstore/examples
$ make
```

Using the Numstore REPL
------------------------

This is a work in progress.

```
$ ./bin/numstore mydb.db
numstore> create a u32;
{ "Status" : "Ok" }
numstore> create b u32;
{ "Status" : "Ok" }
numstore> get a;
{
    "Status" : "Ok",
    "Name"   : "a",
    "DType"  : "u32",
    "DSize"  : 4,
    "Nelems" : 0,
    "Bytes"  : 0,
    "Root"   : 2
}
numstore> get b;
{
    "Status" : "Ok",
    "Name"   : "b",
    "DType"  : "u32",
    "DSize"  : 4,
    "Nelems" : 0,
    "Bytes"  : 0,
    "Root"   : 3
}
numstore> exit;
```

Documentation
-------------

Open `share/doc/numstore/html/index.html` in a browser - it works whether
you double-click it directly or serve the folder over HTTP, no setup
required. Plain markdown versions of the same docs are in
`share/doc/numstore/markdown/`.

Man Pages
---------

```
$ export MANPATH="$PWD/share/man:$MANPATH"
$ man numstore
$ man ns_var_get
```

License
-------

Apache 2.0. See `LICENSE`. See `CHANGELOG.md` for release history.
