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
> read <VNAME> [RANGE] to <DEST>;
```

Where DEST can be:

* A file
```
> read <VNAME> [RANGE] to out.bin;
```

* Another non sink query
```
> read <VNAME> [RANGE] to (insert <VNAME> <OFFSET>);
> read <VNAME> [RANGE] to (write <VNAME> [RANGE]);
```

* To the console
```
> read <VNAME> [RANGE];
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
