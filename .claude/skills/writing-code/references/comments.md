Comments
========

The code is the main documentation. Names, structure and context carry the
meaning. A comment says why, or names a step of an algorithm. It doesn't
repeat the line below it.

Real examples, good and bad, are in
[good-comments.md](../../shared/good-comments.md). The best one to start with
is `pgr_commit`: its `COMMIT / FLUSH / END` markers name the ARIES steps, and
"2PL shrinking phase" says why the unlock comes last.

Rules
-----

1. Don't duplicate the code. `i = i + 1; // add one to i` makes the file worse.
   It's clutter, and it goes stale.
2. If a variable needs a comment to explain what it holds, rename it. Rewrite
   bad code instead of commenting it.
3. If you can't write a clear comment, the code is probably wrong. Simplify
   it until you can.
4. No in-jokes or unexplained abbreviations. If an abbreviation can't be
   avoided, define it once.
5. Explain code that looks wrong. If something looks redundant but is
   required (a second flush, an extra latch release after a crash), say why,
   or someone will "simplify" it.
6. Cite sources. Copied or adapted code gets a link. Algorithms from papers
   get the reference, e.g. `// ARIES Fig. 10: restart analysis`.
7. Comment bug fixes. When a fix looks odd, record what broke and why, and
   point at the regression test that covers it.
8. Mark unfinished work as `// TODO: <what and why>`, specific enough that
   someone else could pick it up. `// TODO: make it work` is useless.
9. When you change code, update or delete the comments around it. A stale
   comment is worse than none.

Where comments go
-----------------

- Headers are for callers: what the function does, what the parameters mean,
  what it returns on success and on failure. Use `///` and refer to
  parameters as `[name]`.
- `.c` files are for people reading the implementation: the steps of the
  algorithm, invariants, and the reasons behind non-obvious choices.
- Each step of a long function (a state machine, a multi-step protocol) gets
  a short marker comment, so the function can stay in one piece. See
  [simplicity-and-functions.md](simplicity-and-functions.md).
- Tests get a comment saying what property they check, and long ones get
  Setup / Run / Teardown markers. See
  [good-tests.md](../../shared/good-tests.md).
