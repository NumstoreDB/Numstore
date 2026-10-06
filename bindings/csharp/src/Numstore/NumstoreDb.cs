// Copyright 2026 Theo Lincke
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

using System;
using System.Runtime.InteropServices;
using Numstore.Interop;

namespace Numstore;

/// <summary>
/// A numstore database (<c>ns_open</c>).
///
/// Queries are plain strings and are passed to numstore verbatim (they are not
/// treated as printf formats, so <c>%</c> is safe). Every method takes an
/// optional transaction; <c>null</c> is passed to numstore as a NULL <c>txn_t*</c>.
///
/// <code>
/// using var db = NumstoreDb.Open("data.ns");
/// using var tx = db.Begin();
/// db.Execute(tx, "create foo u32");
/// db.Write&lt;uint&gt;(tx, new uint[] { 1, 2, 3 }, "insert foo 0 3");
/// uint[] all = db.ReadAll&lt;uint&gt;(tx, "read foo[0:3]");
/// tx.Commit();
/// </code>
/// </summary>
public sealed unsafe class NumstoreDb : Connection
{
    private NumstoreDb(DbHandle h, string path) : base(h, path) { }

    /// <summary>Opens (or creates) a numstore database.</summary>
    public static NumstoreDb Open(string path)
    {
        ArgumentNullException.ThrowIfNull(path);
        return new NumstoreDb(OpenHandle(Native.ns_open(path), "ns_open", path), path);
    }

    /// <summary>Creates a reusable query plan (<c>ns_plan_fcreate</c>). Dispose it when done.</summary>
    public Plan Prepare(string query)
    {
        ArgumentNullException.ThrowIfNull(query);
        using var db = LeaseDb();
        nint plan = VarArgs.ns_plan_fcreate(db.Ptr, query);
        if (plan == 0)
            throw Errors.Db("ns_plan_fcreate", db.Ptr);
        return new Plan(new PlanHandle(Handle, plan), query);
    }

    /// <summary>
    /// Runs a query that needs no data buffer, e.g. <c>create foo u32</c> or
    /// <c>remove foo[0:]</c> (<c>ns_execute</c>).
    /// </summary>
    /// <returns>The non-negative native return code.</returns>
    public int Execute(Transaction? tx, string query)
    {
        ArgumentNullException.ThrowIfNull(query);
        using var db = LeaseDb();
        using var t = LeaseTx(tx);
        return Errors.Check(VarArgs.ns_execute(db.Ptr, t.Ptr, query), "ns_execute", db.Ptr);
    }

    /// <summary>Gets the variable a query refers to, e.g. <c>get foo</c> (<c>ns_get_var</c>).</summary>
    public Variable GetVariable(Transaction? tx, string query)
    {
        ArgumentNullException.ThrowIfNull(query);
        using var db = LeaseDb();
        using var t = LeaseTx(tx);
        nint v = VarArgs.ns_get_var(db.Ptr, t.Ptr, query);
        if (v == 0)
            throw Errors.Db("ns_get_var", db.Ptr);
        return new Variable(new VarHandle(v));
    }

    /// <summary>
    /// Runs a READ or REMOVE query into <paramref name="dest"/> (<c>ns_read</c>).
    /// </summary>
    /// <returns>The native result (non-negative), as returned by <c>ns_read</c>.</returns>
    public long Read<T>(Transaction? tx, Span<T> dest, string query) where T : unmanaged
    {
        ArgumentNullException.ThrowIfNull(query);
        Span<byte> bytes = MemoryMarshal.AsBytes(dest);
        using var db = LeaseDb();
        using var t = LeaseTx(tx);
        fixed (byte* p = bytes)
            return Errors.Check(VarArgs.ns_read(db.Ptr, t.Ptr, p, (ulong)bytes.Length, query), "ns_read", db.Ptr);
    }

    /// <summary>
    /// Runs an INSERT or WRITE query using <paramref name="src"/> as the data (<c>ns_write</c>).
    /// </summary>
    /// <returns>The native result (non-negative), as returned by <c>ns_write</c>.</returns>
    public long Write<T>(Transaction? tx, ReadOnlySpan<T> src, string query) where T : unmanaged
    {
        ArgumentNullException.ThrowIfNull(query);
        ReadOnlySpan<byte> bytes = MemoryMarshal.AsBytes(src);
        using var db = LeaseDb();
        using var t = LeaseTx(tx);
        fixed (byte* p = bytes)
            return Errors.Check(VarArgs.ns_write(db.Ptr, t.Ptr, p, (ulong)bytes.Length, query), "ns_write", db.Ptr);
    }

    /// <summary>Array overload of <see cref="Write{T}(Transaction?, ReadOnlySpan{T}, string)"/>.</summary>
    public long Write<T>(Transaction? tx, T[] src, string query) where T : unmanaged =>
        Write(tx, (ReadOnlySpan<T>)src, query);

    /// <summary>
    /// Runs a READ or REMOVE query and returns everything it produced, letting
    /// numstore size the buffer (<c>ns_malloc</c>).
    /// </summary>
    public byte[] ReadAll(Transaction? tx, string query) => ReadAll<byte>(tx, query);

    /// <inheritdoc cref="ReadAll(Transaction?, string)"/>
    public T[] ReadAll<T>(Transaction? tx, string query) where T : unmanaged
    {
        ArgumentNullException.ThrowIfNull(query);
        using var db = LeaseDb();
        using var t = LeaseTx(tx);
        ulong len = 0;
        void* buf = VarArgs.ns_malloc(db.Ptr, t.Ptr, &len, query);
        if (buf == null)
            throw Errors.Db("ns_malloc", db.Ptr);
        return MallocBuffer.TakeArray<T>(buf, len, "ns_malloc");
    }
}

internal static unsafe class MallocBuffer
{
    /// <summary>Copies a buffer that numstore allocated with system malloc, then frees it.</summary>
    internal static T[] TakeArray<T>(void* buf, ulong len, string op) where T : unmanaged
    {
        try
        {
            if (len % (ulong)sizeof(T) != 0)
                throw new NumstoreException(op,
                    $"returned {len} bytes, which is not a whole number of {typeof(T).Name} ({sizeof(T)} bytes each)");
            ulong count = len / (ulong)sizeof(T);
            if (count > (ulong)Array.MaxLength)
                throw new NumstoreException(op, $"result of {len} bytes is too large for a managed array");

            var result = GC.AllocateUninitializedArray<T>((int)count);
            fixed (T* dst = result)
                Buffer.MemoryCopy(buf, dst, (long)len, (long)len);
            return result;
        }
        finally
        {
            // NativeMemory.Free is the C runtime's free(), which is what the
            // numstore docs require for ns_malloc / ns_plan_malloc buffers.
            NativeMemory.Free(buf);
        }
    }
}
