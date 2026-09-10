---
title: WALFPRINT
section: 1
header: Numstore Manual
footer: numstore 1.0
date: September 2026
---

# NAME

walpprint - print the record headers of a numstore write-ahead log file

# SYNOPSIS

**walpprint** *FNAME*

# DESCRIPTION

**walpprint** is a low-level diagnostic tool for inspecting a
**numstore**(1) write-ahead log (WAL) file directly. It opens the WAL
at *FNAME* and reads it record by record from the beginning,
printing a one-line summary of each record header as it goes.

Each record is read in sequence and logged along with its log
sequence number (**LSN**), which identifies the record's position in
the log. Reading continues until a record of type **WL_EOF** is
reached, marking the logical end of the log, or until a read fails.

**walpprint** only reads record headers - it does not decode or
print the physical or logical payload a record carries (page images,
undo/redo data, and so on). It exists to give a quick view of a
WAL's structure: how many records it holds, what types they are, and
in what order, without running database recovery.

This tool bypasses the query engine and the pager entirely. It is
intended for debugging, not for normal database access; use
**numstore**(1) for that.

# ARGUMENTS

*FNAME*
: Path to the write-ahead log file to inspect.

# OUTPUT

**walpprint** writes one log line per record to the process's
configured log sink (via **LOG_INFO**), each summarizing a record's
header fields and its LSN. No output is written to standard output;
all output is routed through the internal logging facility.

# EXIT STATUS

**0**
: The tool ran to completion, whether or not every record was read
  successfully. If **wal_open** or a call to **wal_read_next** fails,
  the error is logged and **walpprint** stops reading, but currently
  still returns *0*; check the log output for a logged error if the
  record count looks short.

**-1**
: Incorrect number of command-line arguments.

# EXAMPLES

Print all record headers in a WAL file:

    walpprint mydb.wal

# BUGS

Failures opening the WAL file or reading a record are logged via the
internal error reporting facility and stop the read loop, but are
not currently reflected in the process exit status.

# SEE ALSO

**numstore**(1), **dlread**(1)

# AUTHOR

Written by Theo Lincke.

# COPYRIGHT

Copyright 2026 Theo Lincke. Licensed under the Apache License,
Version 2.0. See *http://www.apache.org/licenses/LICENSE-2.0* for
details.
