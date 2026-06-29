```
function_name(N)            Section Title Here            function_name(N)
```

## NAME

```
   function_name, related_name - one-line summary of what it does
```

## LIBRARY

```
   Library or module name (linker flag, import path, etc.)
```

## SYNOPSIS

```
   #include <header.h>

   return_type function_name(arg_type arg,
                             arg_type arg2);
   return_type related_name(arg_type arg);
```

Feature Test Macro Requirements (or equivalent preconditions):

```
   function_name():
       _SOME_FEATURE >= value
           || /* Since version X: */ _OTHER_FEATURE >= value
```

## DESCRIPTION

```
   function_name() does the thing. Describe arguments inline by
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
   #include <header.h>

   int main(void) {
       /* minimal working example */
       return 0;
   }
```

## SEE ALSO

```
   other_function(2), related_function(3), some_topic(7)
```

## COLOPHON

```
   This page is part of the <project> documentation. Source and
   contribution instructions: <url>. Report issues at <url>.
```

```
project-name X.Y.Z              YYYY-MM-DD              function_name(N)
```
