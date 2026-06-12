Inserting
=========

```
> insert <VNAME> <OFFSET> from <SOURCE>;
```

Where SOURCE can be:

* A file
```
> insert <VNAME> <OFFSET> from file.bin;
```

* Another non sink query
```
> insert <VNAME> <OFFSET> from (read <VNAME> [RANGE]);
```

* Literal
```
> insert <VNAME> <OFFSET> from [0, 1, 2, 3];
```

Reading
=======

```
> read <VREF> [RANGE] to <DEST>;
```

Where DEST can be:

* A file
```
> read <VREF> [RANGE] to out.bin;
```

* Another non sink query
```
> read <VREF> [RANGE] to (insert <VNAME> <OFFSET>);
> read <VREF> [RANGE] to (write <VNAME> [RANGE]);
```

* To the console
```
> read <VREF> [RANGE];
```

Examples
--------
```
read foo [0:10];
read foo [0:10] to (write bar [0:10]);
read foo [0:10] to (insert bar 0);
read foo.a [0:10];

# Read into a new struct type
read { a foo.a, b foo.b, c foo.d } to (insert bar 0);

# Read into a new sarray type
read [ foo.a, foo.b[0:10], foo.d ] to (insert bar 0);

# Read into a new combined type
read struct { a foo.b, b [ foo.c, foo.d ] } to (insert bar 0);
```

For the last two examples - we use a type builder syntax. 
You can build sub types into structs or sarrays.

The following type builder:
```
{ a foo.a, b foo.b[0:10], c foo.d };
```

Is an derrived type with the form:
```
struct { a typeof(foo.a), b typeof(foo.b[0:10]), c typeof(foo.d) };
```

For example if foo is:
```
create foo struct { a u32, b [100]f32, c i32, d u64 };
```

Then our derrived type would be:
```
struct { a u32, b [10]f32, c u64 };
```

The following type builder:
```
[ foo.a, foo.b, foo.d ];
```

Requires that foo.a foo.b and foo.d have the same type. It creates a new implicit type:

```
[3] typeof(foo.a);
```

For example if foo is:
```
create foo struct { a i32, b i32, c u64, d i32};
```

Then we would read a derrived type:
```
[3] i32;
```

You can nest structs and sarrays as deeply as you'd like:

```
read struct { a foo.b, b [ foo.c, foo.d ] } to (insert bar 0);
```

Reads:
```
read struct { a typeof(foo.b), b [2] typeof(foo.c) } to (insert bar 0);
```

Note that `foo.c` and `foo.d` must be the same type.

If you're ever confused what type a derrived type is, you can just call `get`:

```
get { a foo.a, b foo.b[0:10], c foo.d };
```

Writing
=======

```
> write <VNAME> [RANGE] from <SOURCE>;
```

Where SOURCE can be:

* A file
```
> write <VNAME> [RANGE] from file.bin;
```

* Another non sink query
```
> write <VNAME> [RANGE] from (read <VNAME> [RANGE]);
```

* Literal
```
> write <VNAME> [RANGE] from [0, 1, 2, 3];
```

Removing
========

```
> remove <VNAME> [RANGE] to <DEST>;
```

Where DEST can be:

* A file
```
> remove <VNAME> [RANGE] to out.bin;
```

* Another non sink query
```
> remove <VNAME> [RANGE] to (insert <VNAME> <OFFSET>);
> remove <VNAME> [RANGE] to (write <VNAME> [RANGE]);
```

* Ignore 
```
> remove <VNAME> [RANGE];
```

* To the console 
```
> remove <VNAME> [RANGE] to console;
```


Variables
=========

```
> create <VNAME> <TYPE>;
> delete <VNAME>;
> get <VNAME GLOB>;
```

Meta Commands
=============

```
> exit;
> help;
> help [COMMAND];
```
