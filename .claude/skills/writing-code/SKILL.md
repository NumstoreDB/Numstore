---
name: writing-code
description: Style, simplicity, function design, assertions and commenting rules for C code in Numstore. Use whenever you write, modify or review C code under csrc/, including tests, so new code reads like the surrounding code.
---

Writing code in Numstore
========================

Match the file you're in first. The rules below are for when the surrounding
code doesn't settle the question.

| Reference                                                              | Covers                                            |
| ---------------------------------------------------------------------- | ------------------------------------------------- |
| [simplicity-and-functions.md](references/simplicity-and-functions.md)  | writing, splitting and merging functions          |
| [assertions.md](references/assertions.md)                              | preconditions, invariants, impossible cases       |
| [comments.md](references/comments.md)                                  | comments, doc comments, TODOs                     |
| [conventions.md](references/conventions.md)                            | formatting, naming, errors, headers               |

Tests have their own skill, [writing-c-unit-tests](../writing-c-unit-tests/SKILL.md).
For WAL, commit, rollback, checkpoint or recovery code, also read
[working-with-aries-logic](../working-with-aries-logic/SKILL.md).

Summary
-------

Functions capture intent.

- Write the most straightforward code that works. No abstraction for a single
  use, no options for cases that don't exist yet.
- A long, straightforward function (a state machine, a multi-step algorithm
  with each step commented) is better than several small ones that mean
  nothing on their own.
- Split along the system's concepts. If the honest name for a function lists
  its steps, it isn't a concept, so inline it.

Assert the program's state.

- Preconditions at the top, expected state after transitions.
- One `DEFINE_DBG_ASSERT` invariant per struct, called with `DBG_ASSERT` at
  the entry of every function that takes it.
- `UNREACHABLE ()` for impossible arms; switch over enums without `default`.
- Assertions are for bugs. Failures in the world are `err_t` errors.

Comments say why.

- Don't restate the code. Rename instead of explaining a bad name.
- Mark the steps of long algorithms, explain code that looks wrong, cite
  papers, and record the cause next to bug fixes.

Follow the conventions.

- `.clang-format`, `name (args)`, module prefixes (`pgr_`, `wal_`, ...).
- `err_t` plus `error *e` last; `error_causef` to start, `error_trace` to
  propagate; clean up in reverse order on failure. Check failure with `< 0`.
- Compare pointers with `== NULL` / `!= NULL`, and print fixed-width types
  with their `PRI` macro.
- Debug-only code and tests under `#ifndef NDEBUG`.

Before finishing
----------------

1. Does it look like the code around it (spacing, naming, error handling)?
2. Is every extracted function a real concept with a short, honest name?
3. Are preconditions and struct invariants asserted, and only bugs asserted?
4. Does every fallible path clean up and `error_trace`?
5. Does any comment restate the code, or no longer match it?
6. Is any non-obvious choice left unexplained?
