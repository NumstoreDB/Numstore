---
name: cleanup
description: Incremental production cleanup pass over Numstore's C code - fix leaks, `< 0` error checks, assertions, PRI format macros, overflow-safe arithmetic, comments, test narration, struct init order, explicit NULL checks, error tests, markdown style, test database names and clang-tidy findings. Use when the user asks to clean up the code, tidy it for production, or do a cleanup pass.
---

Cleaning up Numstore
====================

Do a little of each item below. Don't rewrite anything. Leaks come first
because they're the only item that can be finished. Every change has to
preserve behavior. Run `make test-unit` after each batch.

Style rules are in the [writing-code](../writing-code/SKILL.md) skill, test
rules in [writing-c-unit-tests](../writing-c-unit-tests/SKILL.md).

1. Fix memory leaks (finish this one)
-------------------------------------

Run the leak checker for the platform and fix everything it reports:

| Platform | Command              |
| -------- | -------------------- |
| macOS    | `make leaks-unit`    |
| Linux    | `make valgrind-unit` |

Both use the memcheck build (`-Wall -Wextra -Werror`), so unused variables,
parameters and labels break it. Fix those first.

> `make leaks-unit` won't rebuild a memcheck binary that already exists, so
> after editing sources it can run stale code and pass. Run
> `cmake --build build/memcheck -j12` first.

The rest of the items are never done. Do a few of each per pass.

2. Error checks compare with `< 0`
----------------------------------

Errors are negative `err_t` codes (`common/ns_error.h`). Test for failure with
`< 0`, not by truthiness:

```c
// Before
if (fpgr_extend (p->fp, npages, e)) {
  return error_trace (e);
}

// After
if (fpgr_extend (p->fp, npages, e) < 0) {
  return error_trace (e);
}
```

Check the callee returns `err_t` first. Some look alike but return `bool`
(`faulty_mem_roll`).

For values held in a wider type, see item 14.

3. Add assertions
-----------------

Assert preconditions, postconditions and invariants: arguments, struct
invariants (`DEFINE_DBG_ASSERT` / `DBG_ASSERT`), and the result of a
calculation. A public function that takes a struct with an invariant and
doesn't `DBG_ASSERT` it is an easy find. See
[assertions.md](../writing-code/references/assertions.md).

If a struct is missing a DEFINE_DBG_ASSERT, add it, and add that DBG_ASSERT
to other places in the function. You can make the DBG_ASSERT body fleshed out

4. Format with PRI macros
-------------------------

Print fixed-width types with their `<inttypes.h>` macro, or the project's own
(`PRpgno`, `PRlsn`, ...), never a bare `%d` or `%lu`:

```c
i_log_info ("npages = %" PRIu32 "\n", npages);
```

The macro must match the type, signedness included. A `u32` printed with
`PRIu64` is wrong, and so is an `i64` printed with `PRIu64`: a negative count
prints as a huge number. Look up the declared type of each argument.

5. Overflow-safe arithmetic
---------------------------

Where a sum or product can overflow (sizes, offsets, counts from disk or the
user), use the helpers in `common/ns_bounds.h` (`safe_add_u32`, `safe_mul_u32`,
...) and report `ERR_ARITH` on overflow:

```c
u32 total;
if (!safe_mul_u32 (&total, nelem, size)) {
  return error_causef (e, ERR_ARITH, "allocation size overflows");
}
```

If the existing check already returns a different code (some allocators use
`ERR_NOMEM`), keep that code. Tests may depend on it.

A limit check like `used + size > limit` can wrap. When an invariant
guarantees `used <= limit`, write `size > limit - used` instead, and say so in
a comment.

6. Fix comments
---------------

Delete comments that restate the code, are wrong, or are stale. Where the
line needs one, write a comment that says why. Rules are in
[comments.md](../writing-code/references/comments.md).

7. Narrate tests
----------------

Split long tests into setup, run and teardown, with a short comment for each.
Break up very long assertion expressions into named locals.

```c
// Setup - a pager with one committed page
...

// Run
...

// Teardown
...
```

The Setup comment should describe the state, and the Run comment the behavior
under test. See the test rules in [writing-good-tests](../writing-c-unit-tests)

8. Initialize structs in declaration order
------------------------------------------

Field assignments and designated initializers follow the order the struct
declares its fields:

```c
struct foo
{
  int a;
  int b;
};

// Before
foo.b = b;
foo.a = a;

// After
foo.a = a;
foo.b = b;
```

9. Explicit NULL checks
-----------------------

Compare pointers with `== NULL` / `!= NULL` instead of testing truthiness:

```c
// Before
if (w->ostream) { ... }

// After
if (w->ostream != NULL) { ... }
```

Many fields that look like pointers are `bool` (`save_type`, `in_txn`,
`matches`). Comparing a `bool` to `NULL` compiles and behaves the same in the
debug build, but `-Werror` rejects it. Build the memcheck target (see item 1)
to catch any you got wrong.

10. Add one or two error tests
------------------------------

Pick a function that can fail on the OS or the allocator and has no fail_after
test, and add one, following
[error-tests.md](../writing-c-unit-tests/references/error-tests.md). Check
the output shows at least one `Fail after: N` case actually failing. A sweep
that succeeds at `N = 0` tests nothing.

If the test exposes a bug, don't fix it in a cleanup pass. Document it in a
`TODO` comment and use `TEST_DISABLED`, like `pgr_checkpoint_failure`.

11. Markdown style
------------------

Bring Markdown files in line with the [markdown](../markdown/SKILL.md) skill:
setext headings, 80-column wrap, aligned tables, labeled code blocks.

12. One test database name
--------------------------

Tests name their database `"test"` (nsdb-level tests) or `"testdb"`
(pager-level tests). The goal is a single `TEST_DATABASE_NAME` used
everywhere. Move tests towards one name, and introduce the constant once most
of them agree. Don't move tests whose file names have to differ from each
other (tests that open two databases, or that must not collide).

13. Clean up some clang-tidy findings
-------------------------------------

Run clang-tidy on a few files, using the `.clang-tidy` config in the repo:

```sh
/opt/homebrew/opt/llvm/bin/clang-tidy -p . libs/common/ns_foo.c
```

These are usually worth fixing:

- `bugprone-implicit-widening-of-multiplication-result`: cast one operand to
  `size_t` before multiplying.
- `readability-isolate-declaration`: one declaration per line.
- `clang-analyzer-*`: read each one. Some are false positives, and some are
  real (a `memcpy` to a `NULL` destination with length 0 is still undefined).
  Note real ones for later rather than fixing them in a cleanup pass.

Skip `readability-identifier-naming` and `misc-use-internal-linkage` on the
generated `__test__*` functions.

14. Cast error codes with `err_t_cast`
-------------------------------------

An `i64`, `sb_size` or `int` that holds an error code gets converted to `err_t`
with `err_t_cast (x)`, not a plain `(err_t)` cast or an implicit conversion.
`err_t_cast` asserts the value is `SUCCESS` or a valid error code:

```c
// Before
err_t ret = nsdb_plan_execute (plan, db->tx, e);

// After
err_t ret = err_t_cast (nsdb_plan_execute (plan, db->tx, e));
```

To find them, compile with `-Wshorten-64-to-32` and look for warnings that
mention `err_t`.

Only cast values that really are `SUCCESS` or an error. If the value can be a
positive count (bytes read, elements inserted), `err_t_cast` will assert, and
the real problem is the type: the variable or a static function's return type
should be `i64` / `sb_size`. Fixing that type is fine. Changing a public API's
return type isn't; leave a `TODO` instead.

15. Keep function coverage at 100%
----------------------------------

Every function must be called by at least one unit test. That's a hard rule;
line coverage is best effort. Look for functions marked "not called" in the
coverage report's function list (`build/coverage/html/index.functions.html`
after `make coverage`), and write a test that calls each one. Don't delete an
uncalled function just to raise the number. A print function can be tested
by printing to the log.

Most of the rules below have a clang-tidy check configured in `.clang-tidy`.
Run it on a file and work through what it reports.

16. Signed/unsigned comparisons
-------------------------------

C converts the signed side of a mixed comparison to unsigned, so `-1 < 10U`
is false. Never compare signed and unsigned implicitly. Since `< 0` is how we
check errors, a signed value that has passed that check (or that an invariant
says can't be negative) gets an `ASSERT (x >= 0)`, then an explicit cast:

```c
// Before
if (n < len) ...                 // n is i64, len is u64

// After
ASSERT (n >= 0);
if ((u64)n < len) ...
```

If it can really be negative there, check it: `n >= 0 && (u64)n < len`.
`-Wsign-compare` finds these.

17. No silent narrowing
-----------------------

Every conversion that can lose bits (`u64` → `u32`, `i64` → `int`, signed ↔
unsigned) is explicit. For each one:

- If it's a mistake (the destination type is just wrong), fix the type.
- If the value provably fits, `ASSERT` that it fits, then cast.
- If it might not fit and the function already returns errors, check with
  the `common/ns_bounds.h` helpers and return `ERR_ARITH`. If the function
  doesn't return errors, assert and leave a `TODO` (don't add an error path).

`bugprone-narrowing-conversions` and `-Wshorten-64-to-32` find these.

18. Check every fallible call (outside tests)
---------------------------------------------

In implementation code, every call that returns `err_t` has its result
checked, or is cast to `(void)` with a comment saying why it can't fail or
doesn't matter. Tests may ignore results so they stay readable.
`bugprone-unused-return-value` (configured for `err_t`) finds these.

19. Uppercase literal suffixes
------------------------------

`10U`, `0x7CU`, `1.0F`, `5ULL`, never `10u`. Run
`readability-uppercase-literal-suffix` with `-fix`.

20. Braces on every body
------------------------

Every `if`, `else`, `for` and `while` body has braces, even one line.
`readability-braces-around-statements` finds these.

21. Widen before multiplying, `sizeof *ptr`
-------------------------------------------

Cast an operand before the multiply, not the product: `(u64)a * b`, never
`(u64)(a * b)`. `bugprone-implicit-widening-of-multiplication-result` and
`bugprone-misplaced-widening-cast` find these.

In allocations and copies, size from the pointer, not the type, so the size
follows the pointer if its type changes:

```c
// Before
struct foo *f = malloc (sizeof (struct foo));

// After
struct foo *f = malloc (sizeof *f);
```

22. TODOs and test assertions
-----------------------------

- Every `TODO` says what and why. Rewrite vague ones (`// TODO - fix`) or
  delete them if they're done.
- Tests use typed assertions so failures print both values:
  `test_assert_int_equal (a, b)` or `test_assert_type_equal (a, b, u64,
  PRIu64)`, not `test_assert (a == b)`.

24. Static internal linkage
---------------------------

- If the function is only ever used inside the file, (e.g. it's a helper)
  make it static

25. Safe string operations
---------------------------

- Replace all occurences of strlen with strnlen and bound by a sensible number
  (like MAX_FILE_SIZE or something)
- Replace all

26. Strict aliasing compliance

Avoid violating C strict aliasing rules through illegal pointer type-punning.
When deserializing binary page headers or raw log records from byte arrays, use
memcpy or access through char * pointers rather than casting incompatible
pointer types directly:

```c
// Before (Undefined Behavior under strict aliasing)
struct ns_page_header *hdr = (struct ns_page_header *)raw_bytes;

// After (Compliant and portable)
struct ns_page_header hdr;
memcpy (&hdr, raw_bytes, sizeof (hdr));
```

27. Explicit loop bounds and corruption checks

```c
u32 offset = 0;
while (offset < page_payload_len) {
  if (safe_add_u32 (&offset, offset, record_size)) {
    return error_causef (e, ERR_CORRUPT, "record offset overflow");
  }
}
```

Every loop iterating over variable-length disk records, transaction logs, or
B-tree slots must include explicit upper-bound safety checks against remaining
buffer lengths. Never trust that on-disk length headers are uncorrupted: If a
loop doesn't have a defined upper bound, define a macro above like
MAX_RETRY_LOOPS and loop to that upper bound - return errors for over

36. Stack-preserving error tracing (error_trace)
Never return a raw error code up the call stack if it discards context. Wrap or trace errors at every architectural layer boundary using the project's error-handling mechanisms so that diagnostic logs retain the complete call path down to the root failure:

```c
err_t ret = ns_pager_flush_page (pager, page, e);
if (ret < 0) {
  return error_trace (e); // Preserves file and line context across stack frames
}
```

38. Documented lifetime contracts for pinned page pointers
When returning pointers to cached database pages or pinned slots, document the
precise lifetime contract: the pointer remains valid only while the underlying
page latch or buffer pool pin is held. Ensure the pin is explicitly released as
soon as the transaction or search operation completes:

Before finishing
----------------

1. Do `make test-unit` and a freshly rebuilt memcheck build both pass?
2. Is every change behavior-preserving?
3. Summarize what each item got, any bugs you noticed but didn't fix, and
   what's left for next time.
