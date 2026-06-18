Reading
=======

```
> read <VNAME> [RANGE];
```

Examples

* Read all of foo to the console (with dots):
```
> read foo;
```

* Read a subset of foo;
```
> read foo[10:100];
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
