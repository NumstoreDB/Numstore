---
title: NSSPPRINT
section: 1
header: Numstore Manual
footer: numstore 1.0
date: September 2026
---

# NAME

nsspprint - print raw page contents from a numstore file, bypassing the pager cache

# SYNOPSIS

**nsspprint** *FNAME*

# DESCRIPTION

**nsspprint** is a low-level diagnostic tool for inspecting the pages
of a **numstore**(1) database file directly. It is the "simple"
counterpart to **nspprint**(1): rather than going through the full
pager, it reads every page in *FNAME* straight off disk with the
file pager, in page-number order, and logs a summary of each one.

Because it reads through the file pager rather than the caching
pager used by **nspprint**(1), **nsspprint** reflects exactly what
is on disk with no in-memory page-type filtering, page selection, or
type flags - every page in the file is read and logged
unconditionally.

This tool bypasses the query engine and the caching pager entirely.
It is intended for debugging, not for normal database access; use
**numstore**(1) for that.

# ARGUMENTS

*FNAME*
: Path to the **numstore**(1) database file to inspect.

# OUTPUT

For each page in the file, **nsspprint** writes a summary to the
process's configured log sink at **LOG_INFO** level. No output is
written to standard output on success.

# EXIT STATUS

**0**
: The argument count was correct and the page scan ran to
  completion. Note that a failure reading an individual page is
  logged and that page is skipped, but does not by itself change the
  exit status; check the log output if the page count looks short.

**-1**
: The argument count was wrong. The usage line is also printed to
  standard output in this case.

# EXAMPLES

Print every page in a database file:

    nsspprint mydb

# BUGS

The return value of **fpgr_open** is not checked before the file
pager handle is used to call **fpgr_get_npages** and **fpgr_close**.
If opening *FNAME* fails (for example, a nonexistent path or a
permissions error), this dereferences a null file-pager handle rather
than failing gracefully as **nspprint**(1) and the other diagnostic
tools do.

The **usage** line printed by this program does not currently match
its installed name; verify the two agree if this discrepancy has not
yet been fixed in the source.

# SEE ALSO

**[numstore(1)](/man/man1/numstore.1.md)**, **nspprint**(1)

# AUTHOR

Written by Theo Lincke.

# COPYRIGHT

Copyright 2026 Theo Lincke. Licensed under the Apache License,
Version 2.0. See *http://www.apache.org/licenses/LICENSE-2.0* for
details.
