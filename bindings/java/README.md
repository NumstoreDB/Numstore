# numstore Java bindings (JNI)

Java API over `numstore.h`, backed by a small JNI library (`libnumstore_jni`).

```
src/main/java/com/numstore/   Numstore, SmartFile, Store, Txn, Plan, Var, NumstoreException, NativeLib
src/main/c/numstore_jni.c     JNI glue
CMakeLists.txt                builds numstore.jar + libnumstore_jni
```

## Build

```sh
cmake -B build -DNUMSTORE_INCLUDE_DIR=/path/to/include \
               -DNUMSTORE_LIBRARY=/path/to/libnumstore.so
cmake --build build
# -> build/numstore.jar, build/libnumstore_jni.so
```

Or drop this directory into the numstore tree and `add_subdirectory()` it after
the `numstore` target exists; it links against that target automatically.
A static `libnumstore.a` works if built with `-fPIC`. Requires JDK 11+ and CMake 3.16+.

Javac generates the JNI header during the build and the C file includes it, so a
Java/C signature mismatch is a compile error rather than an `UnsatisfiedLinkError`.

## Run

```sh
java -cp build/numstore.jar:app.jar -Djava.library.path=build MyApp
# or point at the exact file:
java -Dnumstore.jni.library=/abs/path/libnumstore_jni.so ...
```

## Usage

```java
try (Numstore db = Numstore.open("data.ns");
     Txn tx = db.begin()) {
  db.execute(tx, "create foo u32");

  ByteBuffer src = ByteBuffer.allocateDirect(40).order(ByteOrder.nativeOrder());
  for (int i = 0; i < 10; i++) src.putInt(i);
  src.flip();
  db.write(tx, src, "insert foo 0 10");

  int[] out = new int[10];
  ByteBuffer.wrap(db.readAll(tx, "read foo[0:10]"))
            .order(ByteOrder.nativeOrder()).asIntBuffer().get(out);

  try (Var v = db.getVar(tx, "get foo")) { System.out.println(v.length()); }
  tx.commit();                 // Txn.close() rolls back if not committed
}

try (SmartFile f = SmartFile.open("data.smf");
     Txn tx = f.begin()) {
  f.insert(tx, 0, bytes);
  f.read(tx, dest, /*size*/ 4, /*byteOffset*/ 4, /*stride*/ 2, /*nelem*/ 3);
  tx.commit();
}
```

## Mapping and behaviour

| C | Java |
|---|---|
| `ns_open` / `ns_smfile_open` / `ns_cleanup` | `Numstore.open`, `SmartFile.open`, `Store.cleanup` |
| `ns_close` / `ns_crash` | `close()` (idempotent) / `crash()` |
| `ns_begin` / `ns_commit` / `ns_rollback` | `store.begin()`, `txn.commit()`, `txn.rollback()` |
| `ns_execute`, `ns_get_var`, `ns_read`, `ns_write`, `ns_malloc` | `execute`, `getVar`, `read`, `write`, `readAll` |
| `ns_plan_*` | `db.plan(query)` → `Plan` with the same methods |
| `ns_var_len` / `ns_var_free` | `Var.length()` / `Var.close()` |
| `ns_smfile_*` | `SmartFile.size/insert/write/read/remove` |
| `ns_strerror` / `ns_plan_strerror` | message of the thrown `NumstoreException` |

- **Queries** are plain Java strings, passed to C as `("%s", query)`, so `%` in a
  query is never treated as a format directive. Use `String.format` to build them.
- **Buffers**: every read/write takes `byte[]` (optionally with offset/length) or a
  `ByteBuffer` (its `position..limit`; position is not changed). Direct buffers are
  zero-copy. Arrays go through a native scratch copy rather than being pinned, since
  numstore calls can block. Data is raw native byte order.
- **Errors**: a negative `int`/`sb_size` or a NULL pointer throws `NumstoreException`.
  If numstore uses a different convention, change `NS_FAILED` at the top of
  `numstore_jni.c`. `ns_malloc` returning NULL is always treated as an error.
- **Lifetimes**: plans still open when the database closes are freed first (the C API
  requires this). Vars are independent of the database. Using a closed handle throws
  `IllegalStateException` instead of touching freed memory. Passing `null` as the
  `Txn` passes NULL through to numstore.
- **Threads**: closing a store while another thread is mid-call on it is a caller
  error, as in C. As in C, `close()` blocks while a transaction is still open.
