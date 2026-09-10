---
title: NSPPRINT
section: 1
header: Numstore Manual
footer: numstore 1.0
date: September 2026
---

# NAME

nspprint - print raw page headers from a numstore database file

# SYNOPSIS

**nspprint** *FNAME* [*PGNO* ...] [**DL**] [**IN**]

# DESCRIPTION

**nspprint** is a low-level diagnostic tool for inspecting the pages
of a **numstore**(1) database file directly. It opens the pager for
*FNAME*, walks every page in the file in page-number order, and
logs a header summary for each page that matches the selection
criteria given on the command line.

By default, with no *PGNO* arguments and no type flags, **nspprint**
selects every page in the file regardless of type. Selection narrows
in two independent ways, and both may be combined:

- Listing one or more *PGNO* values restricts output to just those
  page numbers.
- Giving one or both of the **DL** / **IN** flags restricts output
  to pages of the matching type(s).

*PGNO* arguments and **DL** / **IN** flags may appear in any order
after *FNAME*, and are only distinguished by whether an argument
parses as an unsigned integer or matches one of the flag keywords.

This tool bypasses the query engine entirely and reads pages
directly through the pager layer. It is intended for debugging, not
for normal database access; use **numstore**(1) for that.

# ARGUMENTS

*FNAME*
: Path to the **numstore**(1) database file to inspect.

*PGNO*
: A page number to include in the output. May be given more than
  once to select several specific pages. If omitted entirely, all
  page numbers are eligible (subject to any type flags given).

# TYPE FLAGS

**DL**
: Restrict output to **DATA_LIST** pages.

**IN**
: Restrict output to inner-node pages.

Both flags may be given together to select pages of either type. If
neither is given, pages of any type are eligible (subject to any
*PGNO* selection given).

# OUTPUT

For each selected page, **nspprint** writes a header summary to the
process's configured log sink at **LOG_INFO** level. No output is
written to standard output on success.

# EXIT STATUS

**0**
: Arguments were valid and the page scan ran to completion. Note
  that failures encountered while opening the pager or fetching an
  individual page are logged and cause that step to be skipped, but
  do not by themselves change the exit status; check the log output
  if the page count looks short.

**-1**
: No arguments were given, an argument could not be parsed (an
  unrecognized flag, an out-of-range or non-numeric *PGNO*), or
  memory could not be allocated while collecting the *PGNO* list.
  The usage line is also printed to standard output in this case.

# EXAMPLES

Print every page in a database file:

    nspprint mydb

Print only pages 3, 7, and 12:

    nspprint mydb 3 7 12

Print only DATA_LIST pages:

    nspprint mydb DL

Print only inner-node pages numbered 10 through 12:

    nspprint mydb IN 10 11 12

# BUGS

If a page is successfully fetched but cannot be released back to the
pager, **nspprint** logs the error and calls **abort**(3),
terminating the process immediately rather than returning a
diagnosable exit status.

The dynamically grown array of requested *PGNO* values is never
freed on the success path; it is only freed on the two argument-
parsing error paths. This is a minor, one-shot leak since the
process exits immediately afterward, but is worth noting for anyone
embedding this logic elsewhere.

# SEE ALSO

**numstore**(1), **dlread**(1)

# AUTHOR

Written by Theo Lincke.

# COPYRIGHT

Copyright 2026 Theo Lincke. Licensed under the Apache License,
Version 2.0. See *http://www.apache.org/licenses/LICENSE-2.0* for
details.
