'use strict';

const test = require('node:test');
const assert = require('node:assert/strict');
const ns = require('..');
const { Database, SmartFile, Transaction } = ns;

const STATE = { code: 'ENUMSTORE_STATE' };
const LIB = { code: 'ENUMSTORE' };

function freshDb() {
  const db = new Database(`/tmp/ns-test-${process.pid}-${Math.random()}`);
  db.execute('create foo u32');
  return db;
}

test('open, close and state flags', () => {
  const db = new Database('/tmp/ns-a');
  assert.equal(db.closed, false);
  assert.equal(db.path, '/tmp/ns-a');
  db.close();
  assert.equal(db.closed, true);
  assert.throws(() => db.begin(), STATE);
  assert.throws(() => db.close(), STATE);
  ns.cleanup('/tmp/ns-a');
});

test('open failure is reported', () => {
  assert.throws(() => new Database('/nonexistent-dir/x'), { code: 'ENUMSTORE', message: /failed to open database/ });
  assert.throws(() => new Database(''), TypeError);
});

test('write / read / readAll round trip', () => {
  const db = freshDb();
  db.write('insert foo 0 5', new Uint32Array([10, 20, 30, 40, 50]));

  const dest = new Uint32Array(3);
  assert.equal(db.read('read foo[1:4]', dest), 12);
  assert.deepEqual([...dest], [20, 30, 40]);

  const all = db.readAll('read foo[0:]');
  assert.ok(Buffer.isBuffer(all));
  assert.deepEqual([...new Uint32Array(all.buffer, all.byteOffset, all.length / 4)], [10, 20, 30, 40, 50]);

  assert.equal(db.readAll('read foo[2:2]').length, 0, 'empty read is an empty Buffer');

  // remove via read copies out then deletes
  const removed = new Uint32Array(2);
  db.read('remove foo[0:2]', removed);
  assert.deepEqual([...removed], [10, 20]);
  assert.equal(db.getVar('get foo').length, 3);
  db.close();
});

test('any buffer-like is accepted and respects byteOffset', () => {
  const db = freshDb();
  const backing = new ArrayBuffer(32);
  const view = new DataView(backing, 8, 8);
  view.setUint32(0, 7, true);
  view.setUint32(4, 9, true);
  db.write('insert foo 0 2', view);
  db.write('insert foo 2 1', new Uint32Array([11]).buffer);

  const out = new ArrayBuffer(16);
  db.read('read foo[0:3]', new Uint8Array(out, 4, 12));
  assert.deepEqual([...new Uint32Array(out, 4, 3)], [7, 9, 11]);
  assert.throws(() => db.read('read foo[0:1]', 'nope'), TypeError);
  db.close();
});

test('library errors carry ns_strerror text', () => {
  const db = freshDb();
  assert.throws(() => db.execute('insert foo 0 10'), { ...LIB, message: /needs a buffer/ });
  assert.throws(() => db.read('get foo', new Uint8Array(4)), { ...LIB, message: /not readable/ });
  assert.throws(() => db.execute('remove bar[0:]'), { ...LIB, message: /does not exist/ });
  assert.throws(() => db.execute('create foo\0 u32'), TypeError);
  db.close();
});

test('format strings in queries are never interpreted', () => {
  const db = freshDb();
  db.execute('create %s%s%n u32'); // would crash if used as a format
  assert.equal(db.getVar('get %s%s%n').length, 0);
  db.close();
});

test('explicit transactions commit and roll back', () => {
  const db = freshDb();
  const tx = db.begin();
  assert.ok(tx instanceof Transaction);
  assert.equal(tx.active, true);
  tx.write('insert foo 0 2', new Uint32Array([1, 2]));
  tx.commit();
  assert.equal(tx.active, false);
  assert.throws(() => tx.commit(), STATE);

  const tx2 = db.begin();
  tx2.write('insert foo 0 1', new Uint32Array([99]));
  assert.equal(tx2.getVar('get foo').length, 3);
  tx2.rollback();
  assert.equal(db.getVar('get foo').length, 2);
  db.close();
});

test('transaction(fn) commits, rolls back on throw, rejects async', () => {
  const db = freshDb();
  const n = db.transaction((tx) => {
    tx.write('insert foo 0 3', new Uint32Array([1, 2, 3]));
    return tx.getVar('get foo').length;
  });
  assert.equal(n, 3);

  assert.throws(
    () =>
      db.transaction((tx) => {
        tx.write('insert foo 0 1', new Uint32Array([4]));
        throw new Error('boom');
      }),
    /boom/,
  );
  assert.equal(db.getVar('get foo').length, 3);

  assert.throws(() => db.transaction(async () => {}), /synchronous/);
  db.close(); // would throw if the async attempt leaked an open txn
});

test('close refuses while a transaction is open; crash does not', () => {
  const db = freshDb();
  const tx = db.begin();
  assert.throws(() => db.close(), { ...STATE, message: /1 transaction is still open/ });
  db.crash();
  assert.equal(db.closed, true);
  assert.equal(tx.active, false);
  assert.throws(() => tx.rollback(), STATE);
});

test('transactions are bound to their database', () => {
  const a = freshDb();
  const b = freshDb();
  const tx = a.begin();
  assert.throws(() => b.execute('create z u8', tx), /different database/);
  tx.rollback();
  a.close();
  b.close();
});

test('plans', () => {
  const db = freshDb();
  const ins = db.plan('insert foo 0 2');
  assert.equal(ins.query, 'insert foo 0 2');
  ins.write(new Uint32Array([5, 6]));
  ins.write(new Uint32Array([3, 4]));

  const rd = db.plan('read foo[0:4]');
  const dest = new Uint32Array(4);
  rd.read(dest);
  assert.deepEqual([...dest], [3, 4, 5, 6]);
  assert.deepEqual([...new Uint32Array(rd.readAll().buffer.slice(0))].length > 0, true);
  assert.equal(rd.getVar().length, 4);

  db.transaction((tx) => db.plan('remove foo[0:1]').execute(tx));
  assert.equal(db.getVar('get foo').length, 3);

  ins.free();
  ins.free(); // idempotent
  assert.equal(ins.freed, true);
  assert.throws(() => ins.write(new Uint32Array(2)), STATE);

  assert.throws(() => db.plan('bogus'), LIB);

  // close() frees surviving plans (C API requires plans freed first)
  assert.equal(rd.freed, false);
  db.close();
  assert.equal(rd.freed, true);
  assert.throws(() => rd.read(dest), STATE);
});

test('variables outlive their database', () => {
  const db = freshDb();
  db.write('insert foo 0 4', new Uint32Array(4));
  const v = db.getVar('get foo');
  db.close();
  assert.equal(v.length, 4);
  v.free();
  assert.equal(v.freed, true);
  assert.throws(() => v.length, STATE);
});

test('smart file: examples from numstore.h', () => {
  const sf = new SmartFile('/tmp/ns-smf');
  assert.equal(sf.size(), 0);

  // insert: [0,1,2,3,4,5] + [6,7,8] at 3 -> [0,1,2,6,7,8,3,4,5]
  sf.insert(new Uint8Array([0, 1, 2, 3, 4, 5]), 0);
  sf.insert(new Uint8Array([6, 7, 8]), 3);
  const all = new Uint8Array(9);
  sf.read(all);
  assert.deepEqual([...all], [0, 1, 2, 6, 7, 8, 3, 4, 5]);
  sf.remove(null, { count: 9 });
  assert.equal(sf.size(), 0);

  // u32 [0..5]; write [6,7,8] at byte 4 stride 2 -> [0,6,2,7,4,8]
  sf.insert(new Uint32Array([0, 1, 2, 3, 4, 5]), 0);
  const orig = new Uint32Array(3);
  sf.read(orig, { offset: 4, stride: 2 });
  assert.deepEqual([...orig], [1, 3, 5]);
  sf.write(new Uint32Array([6, 7, 8]), { offset: 4, stride: 2 });
  const after = new Uint32Array(6);
  sf.read(after);
  assert.deepEqual([...after], [0, 6, 2, 7, 4, 8]);

  // remove [6,7,8] back out -> [0,2,4]
  const removed = new Uint32Array(3);
  assert.equal(sf.remove(removed, { offset: 4, stride: 2 }), 3);
  assert.deepEqual([...removed], [6, 7, 8]);
  const left = new Uint32Array(3);
  sf.read(left);
  assert.deepEqual([...left], [0, 2, 4]);
  assert.equal(sf.size(), 12);
  sf.close();
});

test('smart file: bounds and transactions', () => {
  const sf = new SmartFile('/tmp/ns-smf2');
  sf.insert(new Uint8Array(8), 0);
  assert.throws(() => sf.read(new Uint32Array(2), { count: 3 }), { name: 'RangeError', message: /too small/ });
  assert.throws(() => sf.read(new Uint8Array(4), { offset: 6 }), { ...LIB, message: /past the end/ });
  assert.throws(() => sf.remove(null), /count is required/);
  assert.throws(() => sf.read(new Uint8Array(1), { elementSize: 0 }), RangeError);

  const tx = sf.begin();
  tx.insert(new Uint8Array([1, 2]), 0);
  assert.equal(tx.size(), 10);
  assert.throws(() => tx.execute('create x u8'), TypeError);
  tx.rollback();
  assert.equal(sf.size(), 8);
  sf.close();
});

test('handles cannot be confused across kinds', () => {
  const db = freshDb();
  const sf = new SmartFile('/tmp/ns-smf3');
  const tx = sf.begin();
  assert.throws(() => db.execute('create y u8', tx), /different database/);
  tx.rollback();
  sf.close();
  db.close();
});

test('constructors of handle classes are private', () => {
  assert.throws(() => new Transaction(), TypeError);
  assert.throws(() => new ns.Plan(), TypeError);
  assert.throws(() => new ns.Variable(), TypeError);
});

test('constants', () => {
  assert.equal(ns.constants.PAGE_SIZE, 4096);
  assert.equal(ns.constants.END, 2n ** 63n - 1n);
});

test('using / Symbol.dispose', { skip: typeof Symbol.dispose !== 'symbol' }, () => {
  const db = freshDb();
  const tx = db.begin();
  tx[Symbol.dispose]();
  assert.equal(tx.active, false);
  db[Symbol.dispose]();
  assert.equal(db.closed, true);
});

test('garbage collection in any order does not crash', async () => {
  assert.ok(global.gc, 'run with --expose-gc');
  for (let i = 0; i < 200; i++) {
    const db = freshDb();
    db.plan('read foo[0:]');
    db.getVar('get foo');
    const tx = db.begin(); // abandoned: finalizer rolls back
    tx.write('insert foo 0 1', new Uint32Array([i]));
    if (i % 2) db.begin().rollback();
    // db, plan, var and txn all become unreachable together
  }
  for (let i = 0; i < 5; i++) {
    global.gc();
    await new Promise((r) => setImmediate(r));
  }
});
