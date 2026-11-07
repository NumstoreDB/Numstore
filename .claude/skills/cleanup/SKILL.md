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

Errors are negative `err_t` codes (`core/ns_error.h`). Test for failure with
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

To turn a value that isn't already an `err_t` into one, use `err_t_cast (i)`.
It asserts that `i` is `SUCCESS` or a valid error code, then casts.

3. Add assertions
-----------------

Assert preconditions, postconditions and invariants: arguments, struct
invariants (`DEFINE_DBG_ASSERT` / `DBG_ASSERT`), and the result of a
calculation. A public function that takes a struct with an invariant and
doesn't `DBG_ASSERT` it is an easy find. See
[assertions.md](../writing-code/references/assertions.md).

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
user), use the helpers in `core/ns_bounds.h` (`safe_add_u32`, `safe_mul_u32`,
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
[comments.md](../writing-code/references/comments.md). Examples of comments
to keep and comments to delete are in
[good-comments.md](../shared/good-comments.md).

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
under test. See the narrated test in
[good-tests.md](../shared/good-tests.md#a-narrated-unit-test).

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
[error-tests.md](../writing-c-unit-tests/references/error-tests.md) and the
sweep in [good-tests.md](../shared/good-tests.md#a-fail_after-sweep). Check
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
/opt/homebrew/opt/llvm/bin/clang-tidy -p . csrc/core/ns_foo.c
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

Before finishing
----------------

1. Do `make test-unit` and a freshly rebuilt memcheck build both pass?
2. Is every change behavior-preserving?
3. Summarize what each item got, any bugs you noticed but didn't fix, and
   what's left for next time.
