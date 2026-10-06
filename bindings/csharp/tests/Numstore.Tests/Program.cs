// Exercises the bindings against tests/stub/libnumstore.so.
// Run: tests/stub/build.sh && dotnet run --project tests/Numstore.Tests

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Numstore;

internal static partial class Stub
{
    [LibraryImport("numstore", StringMarshalling = StringMarshalling.Utf8)]
    public static partial int stub_counter(string name);

    [LibraryImport("numstore")]
    private static partial nint stub_last_query(nint db);

    public static string LastQuery(Connection c) =>
        Marshal.PtrToStringUTF8(stub_last_query(((Numstore.Interop.DbHandle)typeof(Connection)
            .GetProperty("Handle", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance)!
            .GetValue(c)!).DangerousGetHandle()))!;
}

internal static class Program
{
    private static int _passed, _failed;

    private static void Test(string name, Action body)
    {
        try { body(); _passed++; Console.WriteLine($"  pass  {name}"); }
        catch (Exception e) { _failed++; Console.WriteLine($"  FAIL  {name}\n        {e.GetType().Name}: {e.Message}"); }
    }

    private static void Eq<T>(T expected, T actual, string what = "")
    {
        if (!EqualityComparer<T>.Default.Equals(expected, actual))
            throw new Exception($"{what} expected <{expected}> got <{actual}>");
    }

    private static void SeqEq<T>(IEnumerable<T> expected, IEnumerable<T> actual)
    {
        if (!expected.SequenceEqual(actual))
            throw new Exception($"expected [{string.Join(",", expected)}] got [{string.Join(",", actual)}]");
    }

    private static TEx Throws<TEx>(Action a) where TEx : Exception
    {
        try { a(); } catch (TEx e) { return e; }
        throw new Exception($"expected {typeof(TEx).Name}");
    }

    private static void Collect() { GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect(); }

    private static int Main()
    {
        Console.WriteLine($"varargs convention: {(Numstore.Interop.VarArgs.UseAppleArm64Convention ? "apple-arm64" : "standard")}");

        Test("varargs pass the query through %s verbatim", () =>
        {
            using var db = NumstoreDb.Open("a.ns");
            db.Execute(null, "create foo u32 -- 100% literal %d %s");
            Eq("create foo u32 -- 100% literal %d %s", Stub.LastQuery(db));
        });

        Test("execute failure surfaces ns_strerror and code", () =>
        {
            using var db = NumstoreDb.Open("a.ns");
            var e = Throws<NumstoreException>(() => db.Execute(null, "bad query"));
            Eq(-7L, e.Code);
            Eq("ns_execute", e.Operation);
            if (!e.Message.Contains("syntax error in 'bad query'")) throw new Exception(e.Message);
        });

        Test("open failure throws", () =>
            Throws<NumstoreException>(() => NumstoreDb.Open("missing.ns")));

        Test("cleanup", () =>
        {
            Connection.Cleanup("a.ns");
            Throws<NumstoreException>(() => Connection.Cleanup("missing.ns"));
        });

        Test("read into typed span (5 fixed args + varargs)", () =>
        {
            using var db = NumstoreDb.Open("a.ns");
            using var tx = db.Begin();
            var dest = new byte[6];
            Eq(6L, db.Read<byte>(tx, dest, "read foo[0:6]"));
            SeqEq(new byte[] { 0, 1, 2, 3, 4, 5 }, dest);
            Eq("read foo[0:6]", Stub.LastQuery(db));
            var u = new uint[2];
            Eq(8L, db.Read<uint>(tx, u, "read foo[0:2]"));
            Eq(0x03020100u, u[0]);
        });

        Test("write from typed span", () =>
        {
            using var db = NumstoreDb.Open("a.ns");
            Eq(1L + 2 + 3, db.Write(null, new uint[] { 1, 2, 3 }, "insert foo 0 3"));
            Eq("insert foo 0 3", Stub.LastQuery(db));
            Throws<NumstoreException>(() => db.Write(null, new byte[] { 1 }, "bad write"));
        });

        Test("ReadAll via ns_malloc (4 fixed args + varargs), freed with C free", () =>
        {
            using var db = NumstoreDb.Open("a.ns");
            SeqEq(new uint[] { 10, 20, 30 }, db.ReadAll<uint>(null, "read foo[0:3]"));
            Eq(12, db.ReadAll(null, "read foo").Length);
            Throws<NumstoreException>(() => db.ReadAll<ulong>(null, "read foo")); // 12 bytes is not whole u64s
            Throws<NumstoreException>(() => db.ReadAll(null, "bad"));
        });

        Test("variables", () =>
        {
            Variable v;
            using (var db = NumstoreDb.Open("a.ns"))
            {
                v = db.GetVariable(null, "get foo");
                Eq(7UL, v.Length);
                Throws<NumstoreException>(() => db.GetVariable(null, "bad"));
            }
            Eq(7UL, v.Length); // valid after db close
            v.Dispose();
            Eq(0, Stub.stub_counter("vars"));
        });

        Test("transactions: commit, rollback, dispose rolls back", () =>
        {
            using var db = NumstoreDb.Open("a.ns");
            int c0 = Stub.stub_counter("commits"), r0 = Stub.stub_counter("rollbacks");
            using (var tx = db.Begin()) tx.Commit();
            using (var tx = db.Begin()) tx.Rollback();
            using (var tx = db.Begin()) { /* abandoned */ }
            Eq(c0 + 1, Stub.stub_counter("commits"));
            Eq(r0 + 2, Stub.stub_counter("rollbacks"));
            var done = db.Begin(); done.Commit();
            Throws<InvalidOperationException>(() => done.Commit());
            Throws<InvalidOperationException>(() => db.Execute(done, "x"));
        });

        Test("transaction from another connection is rejected", () =>
        {
            using var a = NumstoreDb.Open("a.ns");
            using var b = NumstoreDb.Open("b.ns");
            using var tx = a.Begin();
            Throws<ArgumentException>(() => b.Execute(tx, "x"));
        });

        Test("plans", () =>
        {
            using var db = NumstoreDb.Open("a.ns");
            using var tx = db.Begin();
            using (var p = db.Prepare("insert foo 0 3"))
            {
                Eq("insert foo 0 3", Stub.LastQuery(db));
                Eq(0, p.Execute(tx));
                Eq(6L, p.Write(tx, new byte[] { 1, 2, 3 }));
                var d = new byte[4]; Eq(4L, p.Read<byte>(tx, d)); SeqEq(new byte[] { 0, 1, 2, 3 }, d);
                SeqEq(new uint[] { 10, 20, 30 }, p.ReadAll<uint>(tx));
                using var v = p.GetVariable(tx); Eq(14UL, v.Length);
            }
            using (var p = db.Prepare("explode"))
            {
                var e = Throws<NumstoreException>(() => p.Execute(tx));
                Eq("ns_plan_execute (-9): plan exploded", e.Message);
            }
            Throws<NumstoreException>(() => db.Prepare("bad plan"));
            tx.Commit();
        });

        Test("db close is deferred until plans and txns are released", () =>
        {
            int dbs = Stub.stub_counter("dbs");
            var db = NumstoreDb.Open("a.ns");
            var plan = db.Prepare("read foo");
            var tx = db.Begin();
            db.Dispose();                       // user closes too early
            Eq(dbs + 1, Stub.stub_counter("dbs")); // still open natively
            Throws<ObjectDisposedException>(() => db.Begin());
            plan.ReadAll(tx);                   // plan + tx still usable
            plan.Dispose();
            tx.Dispose();
            Eq(dbs, Stub.stub_counter("dbs"));
            Eq(0, Stub.stub_counter("violations"));
        });

        Test("finalizers clean up leaked objects in the right order", () =>
        {
            int dbs = Stub.stub_counter("dbs");
            Leak();
            Collect();
            Eq(dbs, Stub.stub_counter("dbs"));
            Eq(0, Stub.stub_counter("violations"));

            static void Leak()
            {
                var db = NumstoreDb.Open("a.ns");
                db.Prepare("read foo");
                db.Begin();
            }
        });

        Test("crash uses ns_crash", () =>
        {
            int c = Stub.stub_counter("crashes");
            var db = NumstoreDb.Open("a.ns");
            db.Crash();
            Eq(c + 1, Stub.stub_counter("crashes"));
        });

        Test("smart file: header examples (insert / strided write / read / remove)", () =>
        {
            using var smf = SmartFile.Open("s.smf");
            using var tx = smf.Begin();

            smf.Insert(tx, new byte[] { 0, 1, 2, 3, 4, 5 }, 0);
            smf.Insert(tx, new byte[] { 6, 7, 8 }, 3);
            var all = new byte[9]; smf.Read<byte>(tx, all, 0);
            SeqEq(new byte[] { 0, 1, 2, 6, 7, 8, 3, 4, 5 }, all);
            smf.Remove<byte>(tx, 0, 9);
            Eq(0L, smf.Size(tx));

            smf.Insert(tx, new uint[] { 0, 1, 2, 3, 4, 5 }, 0);
            Eq(24L, smf.Size(tx));
            Eq(3L, smf.Write(tx, new uint[] { 6, 7, 8 }, byteOffset: 4, stride: 2));
            var u = new uint[6]; smf.Read<uint>(tx, u, 0);
            SeqEq(new uint[] { 0, 6, 2, 7, 4, 8 }, u);

            var odd = new uint[3];
            smf.Remove<uint>(tx, odd, byteOffset: 4, stride: 2);
            SeqEq(new uint[] { 6, 7, 8 }, odd);
            var rest = new uint[3]; smf.Read<uint>(tx, rest, 0);
            SeqEq(new uint[] { 0, 2, 4 }, rest);

            var e = Throws<NumstoreException>(() => smf.Read<uint>(tx, new uint[10], 0));
            Eq("ns_smfile_read (-6): read past end", e.Message);
            tx.Commit();
        });

        Collect();
        Eq(0, Stub.stub_counter("dbs"), "live dbs at exit");
        Console.WriteLine($"\n{_passed} passed, {_failed} failed");
        return _failed == 0 ? 0 : 1;
    }
}
