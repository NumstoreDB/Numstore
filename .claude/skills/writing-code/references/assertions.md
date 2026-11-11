Assertions
==========

Use assertions. They are important. They validate the state of the program,
and writing them is how you define the program as you write it.

- Assert preconditions at the top of a function: sizes are positive, pointers
  are non-null, offsets are in range.
- Assert the state you expect after a transition, before you rely on it.
  After a branch or a loop that is supposed to leave things in a known state,
  say what that state is:

  ```text
  if page_is_held(limit):
      ...
  # the branch above must leave exactly these three states
  assert prev.mode  == NONE
  assert cur.mode   == EXCLUSIVE
  assert next.mode  == NONE
  ```

- Assert the state of structures, not only of arguments. Write an invariant
  check for each data structure, then call it at the entry of every function
  that takes the structure and at the end of every function that builds or
  changes it:

  ```text
  invariant(buffer b):
      assert b != null
      assert b.data != null
      assert b.count <= b.capacity

  append(b, ...):
      invariant(b)
      ...
  ```

- A wall of blanket asserts at the top of a function is okay, but it may mean
  you should assert the state of something instead. If the same four asserts
  keep showing up for the same structure, that is an invariant function that
  hasn't been written yet.
- Mark impossible code as impossible: an `unreachable` marker in the arm of a
  switch that can't happen. Switch exhaustively over enums with no default
  case, so the compiler flags a new enum value that nobody handled.
- Assertions are for bugs in the program, not for failures in the world. Bad
  input, a full disk or a failed allocation is an error to return, never an
  assertion.

In Numstore
-----------

The macros live in `common/ns_csx_assert.h`. All of them compile away under
`NDEBUG`.

| Macro                                      | Use                                         |
| ------------------------------------------ | ------------------------------------------- |
| `ASSERT (expr)`                            | a local precondition or post-state          |
| `ASSERT_NN (ptr)` / `ASSERT_GTE0 (v)`      | inline checks that return their argument    |
| `DEFINE_DBG_ASSERT (type, name, var, {..})`| the invariant function for a struct         |
| `DBG_ASSERT (name, ptr)`                   | call that invariant                         |
| `DBG_ASSERT_IF_NN (name, ptr)`             | call it only when `ptr` is non-null         |
| `UNREACHABLE ()`                           | an arm that can't happen                    |
| `UNIMPLEMENTED ()`                         | a path not written yet                      |

The invariant function sits next to the struct definition:

```c
DEFINE_DBG_ASSERT (struct wal, wal, w, {
  ASSERT (w);
  // ostream can be null
  ASSERT (w->istream);
})
```

and every public function that takes the struct starts with it:

```c
err_t
pgr_commit (struct pager *p, struct txn *tx, error *e)
{
  DBG_ASSERT (pager, p);
  ...
```

Failures in the world go through `error_causef` and `err_t`, never `ASSERT`.
Failures that leave the system unable to continue safely (a WAL write that
can't complete) call `intentionally_crash ()`; that is a deliberate crash, not
an assertion.
