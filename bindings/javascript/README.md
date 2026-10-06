# numstore for Node.js

Node-API bindings for numstore (`include/numstore.h`): databases, transactions, query
plans, variables and smart files. Synchronous, like the C API (and like
better-sqlite3).

## Build

```sh
NUMSTORE_INCLUDE_DIR=/path/to/numstore/include \
NUMSTORE_LIB_DIR=/path/to/numstore/build \
npm install
```

| Variable               | Default                           |
| ---------------------- | --------------------------------- |
| `NUMSTORE_INCLUDE_DIR` | `./include` (copy of `numstore.h`) |
| `NUMSTORE_LIB_DIR`     | system linker path                |
| `NUMSTORE_LIB`         | `numstore` (links `-lnumstore`)   |

On Linux/macOS the library directory is baked into the addon's rpath.

## Usage

```js
const { Database, SmartFile } = require('numstore');

const db = new Database('data.ns');

// No tx argument → the call runs in its own one-shot transaction
db.execute('create foo u32');
db.write('insert foo 0 3', new Uint32Array([1, 2, 3]));

const out = new Uint32Array(3);
db.read('read foo[0:3]', out);          // into your buffer
const buf = db.readAll('read foo[0:]'); // into a new Buffer
db.getVar('get foo').length;            // 3

// Explicit transactions
db.transaction((tx) => {
  tx.write('insert foo 0 2', new Uint32Array([7, 8]));
  tx.execute('remove foo[0:1]');
}); // committed on return, rolled back on throw

const tx = db.begin();
try { /* ... */ tx.commit(); } catch (e) { tx.rollback(); throw e; }

// Plans
const ins = db.plan('insert foo 0 2');
ins.write(new Uint32Array([5, 6]));
ins.free(); // optional: GC and db.close() also free plans

db.close();
```

### Smart files

Offsets are in **bytes**, `stride` and `count` in **elements**, and
`elementSize` defaults to the buffer's `BYTES_PER_ELEMENT`:

```js
const sf = new SmartFile('data.smf');
sf.insert(new Uint32Array([0, 1, 2, 3, 4, 5]), 0);
sf.write(new Uint32Array([6, 7, 8]), { offset: 4, stride: 2 }); // [0,6,2,7,4,8]

const got = new Uint32Array(3);
sf.read(got, { offset: 4, stride: 2 });       // [6,7,8]
sf.remove(null, { offset: 4, stride: 2, count: 3 }); // [0,2,4]
sf.size(); // 12 (bytes)
sf.close();
```

Every method takes an optional trailing `tx`; `Transaction` also has the same
methods bound to itself (`tx.read(...)`, `tx.insert(...)`, ...).

Buffers can be any TypedArray, `Buffer`, `DataView` or `ArrayBuffer`.
Integers can be `number` or `bigint`. `Symbol.dispose` is implemented, so
`using db = new Database(p)` works where supported.

## Errors

Thrown errors carry a `code`:

- `ENUMSTORE` — the library failed; the message comes from `ns_strerror` /
  `ns_plan_strerror`.
- `ENUMSTORE_STATE` — misuse: database closed, transaction already finished,
  plan freed, transaction from another database.
- `TypeError` / `RangeError` — bad arguments (wrong buffer type, buffer too
  small for `count × elementSize`, etc.).

## Safety guarantees

- Native handles are type-tagged; a transaction can't be passed where a
  database is expected, and a transaction can only be used with the database
  that created it.
- Queries are always passed to the C API as `("%s", query)`, so `%` in a query
  is never treated as a format directive.
- Strided smart-file calls are bounds-checked against the JS buffer before
  calling into C.
- Plans are freed before `ns_close` (the C API requires it).
- `close()` **throws** if transactions are still open rather than calling
  `ns_close`, which would block the event loop forever. Use `crash()` to close
  anyway.
- Anything garbage-collected is cleaned up in any order: abandoned
  transactions are rolled back (never committed), plans and variables freed,
  and an unclosed database is closed gracefully.

## Assumptions about the C API

The header doesn't spell these out, so the bindings assume:

1. Failure is signalled by `NULL` pointers and negative `int` / `sb_size`
   returns, and `ns_strerror` describes the most recent failure.
2. `ns_malloc` returning `NULL` with `*dlen == 0` is a successful empty read.
3. A failed `ns_commit` leaves the transaction open (so it can be rolled
   back); a failed `ns_rollback` leaves it finished.
4. `ns_close`/`ns_crash` invalidate the handle even when they return an error.

If any of those is wrong, the relevant spots are marked in `src/binding.c`.

## Tests

```sh
npm run test:stub   # builds test/stub (an in-memory fake libnumstore), links, runs tests
npm test            # runs tests against whatever library the addon is linked to
```

The stub implements a toy version of the query language so the bindings can
be tested without a numstore build; it is not numstore.
