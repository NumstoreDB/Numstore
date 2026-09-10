---
title: QUERY_COMPILE_CLI
section: 1
header: Numstore Manual
footer: numstore 1.0
date: September 2026
---

# NAME

print_query - compile a numstore query string and log its parsed form

# SYNOPSIS

**print_query** *"QUERY"*

# DESCRIPTION

**print_query** compiles a single **numstore**(1) query string
given on the command line and logs the resulting parsed query. It
exercises the compiler front end directly, without opening a
database or executing the query, making it useful for checking
whether a query is syntactically valid and inspecting how it parses.

*QUERY* is compiled with a private, scratch allocator that is torn
down before the process exits, so no database state or memory
persists across invocations.

# ARGUMENTS

*QUERY*
: A single **numstore**(1) query string, using the same syntax
  accepted by the **numstore** REPL (**CREATE**, **GET**, **DELETE**,
  **INSERT**, **WRITE**, **READ**, **REMOVE**). Pass it as one
  shell argument, quoted if it contains spaces.

# OUTPUT

On success, the parsed form of *QUERY* is written via the internal
logging facility at **LOG_INFO** level. No output is written to
standard output.

On failure, a one-line error message naming the offending query
string is written to standard error.

# EXIT STATUS

**EXIT_SUCCESS** (**0**)
: *QUERY* compiled successfully.

**EXIT_FAILURE** (**1**)
: Either the argument count was wrong, or *QUERY* failed to compile.

# EXAMPLES

Check that a query parses, and see its compiled form in the log
output:

    print_query "create foo u32"

Check an array-range query:

    print_query "read foo[0:10:2]"

A malformed query fails and reports the offending string:

    $ print_query "create foo bogustype"
    error: failed to compile query: create foo bogustype

# NOTES

**print_query** logs the parsed query rather than printing it
to standard output; direct it accordingly if capturing output in a
script (for example, by capturing the process's log stream rather
than stdout).

# SEE ALSO

**numstore**(1)

# AUTHOR

Written by Theo Lincke.

# COPYRIGHT

Copyright 2026 Theo Lincke. Licensed under the Apache License,
Version 2.0. See *http://www.apache.org/licenses/LICENSE-2.0* for
details.
