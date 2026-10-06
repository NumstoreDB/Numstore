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
using System.Threading;

namespace Numstore.Interop;

/// <summary>
/// Owns an <c>nsdb_t*</c>. Released with <c>ns_close</c>, or <c>ns_crash</c>
/// when <see cref="CrashOnRelease"/> is set.
///
/// Transactions and plans hold a reference count on this handle, so the
/// database is never closed underneath them — not even by the finalizer.
/// </summary>
public sealed class DbHandle : SafeHandle
{
    private int _crash;

    public DbHandle() : base(IntPtr.Zero, ownsHandle: true) { }

    internal DbHandle(nint ptr) : this() => SetHandle(ptr);

    public override bool IsInvalid => handle == IntPtr.Zero;

    internal bool CrashOnRelease
    {
        get => Volatile.Read(ref _crash) != 0;
        set => Volatile.Write(ref _crash, value ? 1 : 0);
    }

    /// <summary>Result code of the native close, available after release.</summary>
    internal int CloseResult { get; private set; }

    protected override bool ReleaseHandle()
    {
        CloseResult = CrashOnRelease ? Native.ns_crash(handle) : Native.ns_close(handle);
        return CloseResult >= 0;
    }
}

/// <summary>
/// Owns a <c>txn_t*</c>. If the transaction is released without being
/// committed or rolled back explicitly, it is rolled back.
/// </summary>
public sealed class TxnHandle : SafeHandle
{
    private readonly DbHandle _db;
    private int _finished;

    internal TxnHandle(DbHandle db, nint ptr) : base(IntPtr.Zero, ownsHandle: true)
    {
        _db = db;
        bool added = false;
        db.DangerousAddRef(ref added); // keeps the database open while this txn lives
        SetHandle(ptr);
    }

    public override bool IsInvalid => handle == IntPtr.Zero;

    internal DbHandle Db => _db;

    /// <summary>Marks the native txn as already ended (committed / rolled back).</summary>
    internal void MarkFinished() => Volatile.Write(ref _finished, 1);

    internal bool IsFinished => Volatile.Read(ref _finished) != 0;

    protected override bool ReleaseHandle()
    {
        try
        {
            if (!IsFinished)
                Native.ns_rollback(_db.DangerousGetHandle(), handle);
            return true;
        }
        finally
        {
            _db.DangerousRelease();
        }
    }
}

/// <summary>
/// Owns an <c>nsdb_plan_t*</c>. numstore requires plans to be freed before
/// their database is closed; holding a reference on the <see cref="DbHandle"/>
/// enforces that ordering.
/// </summary>
public sealed class PlanHandle : SafeHandle
{
    private readonly DbHandle _db;

    internal PlanHandle(DbHandle db, nint ptr) : base(IntPtr.Zero, ownsHandle: true)
    {
        _db = db;
        bool added = false;
        db.DangerousAddRef(ref added);
        SetHandle(ptr);
    }

    public override bool IsInvalid => handle == IntPtr.Zero;

    internal DbHandle Db => _db;

    protected override bool ReleaseHandle()
    {
        try
        {
            Native.ns_plan_free(handle);
            return true;
        }
        finally
        {
            _db.DangerousRelease();
        }
    }
}

/// <summary>
/// Owns an <c>nsdb_var_t*</c>. Variables live in their own memory and may be
/// freed before or after the database is closed, so no reference is taken.
/// </summary>
public sealed class VarHandle : SafeHandle
{
    public VarHandle() : base(IntPtr.Zero, ownsHandle: true) { }

    internal VarHandle(nint ptr) : this() => SetHandle(ptr);

    public override bool IsInvalid => handle == IntPtr.Zero;

    protected override bool ReleaseHandle()
    {
        Native.ns_var_free(handle);
        return true;
    }
}

/// <summary>
/// Pins a <see cref="SafeHandle"/> for the duration of a native call so it
/// cannot be released concurrently. A null handle leases as <c>NULL</c>.
/// </summary>
internal ref struct Lease
{
    private readonly SafeHandle? _h;
    private bool _added;
    public readonly nint Ptr;

    public Lease(SafeHandle? h)
    {
        _h = h;
        _added = false;
        if (h is null)
        {
            Ptr = 0;
            return;
        }
        h.DangerousAddRef(ref _added); // throws ObjectDisposedException if closed
        Ptr = h.DangerousGetHandle();
    }

    public void Dispose()
    {
        if (_added)
        {
            _h!.DangerousRelease();
            _added = false;
        }
    }
}
