```
smfile_open(3)            Smart Files Man Pages            smfile_open(3)
```

[NAME](#name) | [LIBRARY](#library) | [SYNOPSIS](#synopsis)
| [DESCRIPTION](#description) | [RETURN VALUE](#return-value)
| [ERRORS](#errors) | [STANDARDS](#standards) | [HISTORY](#history)
| [NOTES](#notes) | [BUGS](#bugs) | [EXAMPLES](#examples)
| [SEE ALSO](#see-also) | [COLOPHON](#colophon)

## NAME

```
   smfile_open, smfile_close - open a database under a given file name path
```

## LIBRARY

```
   Standard numstore library (libnumstore, -lnumstore)
```

## SYNOPSIS

```
   #include <numstore.h>

   smfile_t* smfile_open(const char* path);
   int smfile_close(smfile_t* file);
```

Feature Test Macro Requirements (or equivalent preconditions):

## DESCRIPTION

```
   smfile_open() does the thing. Describe arguments inline by
   name: fd is the file descriptor, buf is the destination buffer,
   count is the maximum number of bytes to transfer.

   related_name() does the related thing. Note any shared semantics
   and any way the two diverge.

   Any preconditions on the inputs go here. Any invariants the
   caller must uphold go here.
```

## RETURN VALUE

```
   On success, function_name() returns the number of units
   processed. A return of zero indicates <specific meaning>.

   On error, -1 is returned and errno is set to indicate the
   error.
```

## ERRORS

```
   EINVAL  arg was out of range, or some constraint was violated.

   ENOENT  The named target does not exist.

   EPERM   The caller lacks the required permission.
```

## STANDARDS

```
   POSIX.1-2024, or the relevant spec/RFC.
```

## HISTORY

```
   Added in Linux X.Y. C library support added in glibc Z.
```

Library/kernel differences
Sub-sections within a section are indented less than the body
and titled in sentence case. Describe quirks like syscall
renames, wrapper behavior, or ABI differences here.

## NOTES

```
   Usage guidance, threading considerations, and other context that
   does not fit DESCRIPTION but a reader needs to know.
```

## BUGS

```
   Known divergences from the spec, or known issues. Be specific:
   which platform, which version, which condition.
```

## EXAMPLES

```
   #include <numstore.h>

   int main(void) {
       /* minimal working example */
       return 0;
   }
```

## SEE ALSO

```
   smfile_close(3), smfile_cleanup(3)
```

## COLOPHON

```
   This page is part of the <project> documentation. Source and
   contribution instructions: <url>. Report issues at <url>.
```

```
Numstore Reference pages 0.0.1             2026-06-25              smfile_open (3)
```
