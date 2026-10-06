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
/// A prepared query (<c>nsdb_plan_t*</c>), created with
/// <see cref="NumstoreDb.Prepare"/>. numstore requires plans to be freed before
/// their database closes; the bindings enforce this by deferring the database's
/// native close until every plan has been disposed.
/// </summary>
public sealed unsafe class Plan : IDisposable
{
    internal PlanHandle Handle { get; }

    /// <summary>The query this plan was created from.</summary>
    public string Query { get; }

    internal Plan(PlanHandle handle, string query)
    {
        Handle = handle;
        Query = query;
    }

    /// <summary>The last error reported for this plan (<c>ns_plan_strerror</c>).</summary>
    public string LastError
    {
        get
        {
            using var p = new Lease(Handle);
            return Errors.FromPlan(p.Ptr);
        }
    }

    /// <summary><c>ns_plan_execute</c>: runs a plan that needs no data buffer.</summary>
    public int Execute(Transaction? tx)
    {
        using var p = new Lease(Handle);
        using var t = Connection.LeaseTx(Handle.Db, tx);
        return Errors.CheckPlan(Native.ns_plan_execute(p.Ptr, t.Ptr), "ns_plan_execute", p.Ptr);
    }

    /// <summary><c>ns_plan_get_var</c>: the variable the plan refers to.</summary>
    public Variable GetVariable(Transaction? tx)
    {
        using var p = new Lease(Handle);
        using var t = Connection.LeaseTx(Handle.Db, tx);
        nint v = Native.ns_plan_get_var(p.Ptr, t.Ptr);
        if (v == 0)
            throw Errors.Plan("ns_plan_get_var", p.Ptr);
        return new Variable(new VarHandle(v));
    }

    /// <summary><c>ns_plan_read</c>: runs a READ / REMOVE plan into <paramref name="dest"/>.</summary>
    public long Read<T>(Transaction? tx, Span<T> dest) where T : unmanaged
    {
        Span<byte> bytes = MemoryMarshal.AsBytes(dest);
        using var p = new Lease(Handle);
        using var t = Connection.LeaseTx(Handle.Db, tx);
        fixed (byte* d = bytes)
            return Errors.CheckPlan(Native.ns_plan_read(p.Ptr, t.Ptr, d, (ulong)bytes.Length), "ns_plan_read", p.Ptr);
    }

    /// <summary><c>ns_plan_write</c>: runs an INSERT / WRITE plan from <paramref name="src"/>.</summary>
    public long Write<T>(Transaction? tx, ReadOnlySpan<T> src) where T : unmanaged
    {
        ReadOnlySpan<byte> bytes = MemoryMarshal.AsBytes(src);
        using var p = new Lease(Handle);
        using var t = Connection.LeaseTx(Handle.Db, tx);
        fixed (byte* s = bytes)
            return Errors.CheckPlan(Native.ns_plan_write(p.Ptr, t.Ptr, s, (ulong)bytes.Length), "ns_plan_write", p.Ptr);
    }

    /// <summary>Array overload of <see cref="Write{T}(Transaction?, ReadOnlySpan{T})"/>.</summary>
    public long Write<T>(Transaction? tx, T[] src) where T : unmanaged => Write(tx, (ReadOnlySpan<T>)src);

    /// <summary><c>ns_plan_malloc</c>: runs a READ / REMOVE plan and returns all of its output.</summary>
    public byte[] ReadAll(Transaction? tx) => ReadAll<byte>(tx);

    /// <inheritdoc cref="ReadAll(Transaction?)"/>
    public T[] ReadAll<T>(Transaction? tx) where T : unmanaged
    {
        using var p = new Lease(Handle);
        using var t = Connection.LeaseTx(Handle.Db, tx);
        ulong len = 0;
        void* buf = Native.ns_plan_malloc(p.Ptr, t.Ptr, &len);
        if (buf == null)
            throw Errors.Plan("ns_plan_malloc", p.Ptr);
        return MallocBuffer.TakeArray<T>(buf, len, "ns_plan_malloc");
    }

    /// <summary>Frees the plan (<c>ns_plan_free</c>).</summary>
    public void Dispose() => Handle.Dispose();
}

/// <summary>
/// A numstore variable (<c>nsdb_var_t*</c>). Variables own their memory and
/// may be disposed before or after the database is closed.
/// </summary>
public sealed class Variable : IDisposable
{
    internal VarHandle Handle { get; }

    internal Variable(VarHandle handle) => Handle = handle;

    /// <summary>Length in elements, not bytes (<c>ns_var_len</c>).</summary>
    public ulong Length
    {
        get
        {
            using var v = new Lease(Handle);
            return Native.ns_var_len(v.Ptr);
        }
    }

    /// <summary>Frees the variable (<c>ns_var_free</c>).</summary>
    public void Dispose() => Handle.Dispose();
}
