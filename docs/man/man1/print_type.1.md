---
title: TYPE_COMPILE_CLI
section: 1
header: Numstore Manual
footer: numstore 1.0
date: September 2026
---

# NAME

print_type - compile a numstore type string and print its normalized form

# SYNOPSIS

**print_type** *"TYPE"*

# DESCRIPTION

**print_type** compiles a single **numstore**(1) type
expression given on the command line and prints its normalized,
canonical form to standard output. It exercises the type compiler
directly, without opening a database, making it useful for checking
whether a type expression is valid and seeing how it is rendered
after compilation.

*TYPE* is compiled with a private, scratch allocator that is torn
down before the process exits, so no database state persists across
invocations.

# ARGUMENTS

*TYPE*
: A single **numstore**(1) type expression, using the same syntax
  accepted by **create** in the **numstore** REPL: a primitive
  (**u32**, **f64**, **cf128**, ...), a **struct { ... }**, a
  **union { ... }**, a strict array (**[N]... TYPE**), or any nesting
  of these. See **numstore**(1) for the full type system. Pass it as
  one shell argument, quoted if it contains spaces or braces.

# OUTPUT

On success, the compiled type's canonical string form is written to
standard output, followed by a newline. This form may differ
cosmetically from the input (for example, normalized spacing) even
when the type is unchanged semantically.

On failure, a one-line error message naming the offending type
string is written to standard error.

# EXIT STATUS

**EXIT_SUCCESS** (**0**)
: *TYPE* compiled successfully and its canonical form was printed.

**EXIT_FAILURE** (**1**)
: The argument count was wrong, *TYPE* failed to compile, or the
  canonical string form could not be allocated.

# EXAMPLES

Print the canonical form of a primitive:

    $ print_type "u32"
    u32

Print the canonical form of a nested struct:

    $ print_type "struct { a u32, b cf256 }"
    struct { a u32, b cf256 }

A malformed type fails and reports the offending string:

    $ print_type "struct { a bogustype }"
    error: failed to compile type: struct { a bogustype }

# BUGS

The canonical type string returned by the type compiler is allocated
from the CLI's scratch arena allocator, but is released with a plain
**free**(3) call rather than through that allocator. If the string is
suballocated from arena memory rather than obtained via the system
allocator, this is a mismatched deallocation and undefined behavior,
though it is unlikely to have an observable effect before the
process exits.

# SEE ALSO

**numstore**(1), **print_query**(1)

# AUTHOR

Written by Theo Lincke.

# COPYRIGHT

Copyright 2026 Theo Lincke. Licensed under the Apache License,
Version 2.0. See *http://www.apache.org/licenses/LICENSE-2.0* for
details.
