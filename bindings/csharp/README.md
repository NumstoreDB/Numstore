# Numstore.Net

C# bindings for `numstore.h` (.NET 8+).

```
src/Numstore/
  Interop/Native.cs     raw 1:1 P/Invoke declarations + VarArgs dispatch
  Interop/Handles.cs    SafeHandles for nsdb_t*, txn_t*, nsdb_plan_t*, nsdb_var_t*
  NumstoreDb.cs         ns_open, execute / get_var / read / write / malloc
  SmartFile.cs          ns_smfile_* (insert, strided read/write/remove, size)
  Plan.cs               ns_plan_* and Variable (ns_var_len / ns_var_free)
  Transaction.cs        ns_begin / ns_commit / ns_rollback
tests/
  stub/                 a fake libnumstore implementing the ABI (Linux)
  Numstore.Tests/       console test runner
```

## Usage

```csharp
using Numstore;

using var db = NumstoreDb.Open("data.ns");
using (var tx = db.Begin())
{
    db.Execute(tx, "create foo u32");
    db.Write(tx, new uint[] { 1, 2, 3 }, "insert foo 0 3");
    uint[] all = db.ReadAll<uint>(tx, "read foo[0:3]");   // ns_malloc, freed for you

    var buf = new uint[2];
    db.Read<uint>(tx, buf, "read foo[0:2]");             // ns_read into your buffer

    using var plan = db.Prepare("read foo[0:3]");        // ns_plan_fcreate
    plan.ReadAll<uint>(tx);

    tx.Commit();                                         // disposing uncommitted = rollback
}

using var smf = SmartFile.Open("data.smf");
using (var tx = smf.Begin())
{
    smf.Insert(tx, new uint[] { 0, 1, 2, 3, 4, 5 }, byteOffset: 0);
    smf.Write(tx, new uint[] { 6, 7, 8 }, byteOffset: 4, stride: 2);  // [0,6,2,7,4,8]
    tx.Commit();
}
```

Failures throw `NumstoreException` carrying the `ns_strerror` / `ns_plan_strerror`
message and the native return code. `Numstore.Interop.Native` / `VarArgs` expose
the raw functions if you want to manage pointers yourself.

## Design notes

- **Varargs.** .NET can't call C variadics, so every `fmt, ...` function is
  called as `f(..., "%s", query)`. Queries are therefore never treated as format
  strings (`%` is literal); build them with C# interpolation instead. The fixed
  signature matches the variadic ABI on x86/x64 and Linux/Windows ARM64. Apple
  ARM64 passes variadic args on the stack, so on macOS/iOS ARM64 the bindings
  pad x0–x7 so the query lands in the first stack slot. That path is not
  exercised by the tests here (they ran on Linux x64); worth a run on an
  M-series Mac.
- **Lifetimes.** Plans and transactions hold a reference on the database
  handle, so `ns_close` only runs after they are freed — numstore's
  "free plans before close" rule holds even if the GC finalizes things in
  arbitrary order. Variables hold no reference (they're independent, per the
  header).
- **`ns_malloc` buffers** are released with `NativeMemory.Free` (C `free`). On
  Windows this assumes numstore links the same CRT (ucrt).
- **Assumptions about the C library** (not stated in the header): `int`/`sb_size`
  results `< 0` mean failure; a failed `ns_commit` leaves the transaction open
  (it will be rolled back on dispose); a NULL `txn_t*` is passed through as-is
  when you pass `null`; `ns_malloc` returning NULL is an error.

## Library loading

The bindings import `numstore`, so the runtime looks for `libnumstore.so`,
`libnumstore.dylib` or `numstore.dll` next to your app or on the system library
path. Use `NativeLibrary.SetDllImportResolver` for other locations.

## Tests

```sh
tests/stub/build.sh
dotnet run --project tests/Numstore.Tests
```

The stub records what each call received and implements smart files for real,
so the tests check argument marshalling, varargs, error propagation,
commit/rollback/dispose, deferred close ordering, finalizer cleanup, and the
strided examples from the header.
