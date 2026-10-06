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
using System.Threading;
using Numstore.Interop;

namespace Numstore;

/// <summary>
/// Common lifecycle for anything backed by an <c>nsdb_t*</c>:
/// a <see cref="NumstoreDb"/> or a <see cref="SmartFile"/>.
/// </summary>
public abstract class Connection : IDisposable
{
    internal DbHandle Handle { get; }

    /// <summary>Path the connection was opened with.</summary>
    public string Path { get; }

    private protected Connection(DbHandle handle, string path)
    {
        Handle = handle;
        Path = path;
    }

    private protected static DbHandle OpenHandle(nint ptr, string op, string path)
    {
        if (ptr == 0)
            throw new NumstoreException(op, $"could not open '{path}'");
        return new DbHandle(ptr);
    }

    /// <summary>Removes the numstore database or smart file at <paramref name="path"/> (<c>ns_cleanup</c>).</summary>
    public static void Cleanup(string path)
    {
        ArgumentNullException.ThrowIfNull(path);
        int rc = Native.ns_cleanup(path);
        if (rc < 0)
            throw new NumstoreException("ns_cleanup", $"could not remove '{path}'", rc);
    }

    /// <summary>The last error message reported for this connection (<c>ns_strerror</c>).</summary>
    public string LastError
    {
        get
        {
            using var db = LeaseDb();
            return Errors.FromDb(db.Ptr);
        }
    }

    /// <summary>Begins a transaction (<c>ns_begin</c>).</summary>
    public Transaction Begin()
    {
        using var db = LeaseDb();
        nint tx = Native.ns_begin(db.Ptr);
        if (tx == 0)
            throw Errors.Db("ns_begin", db.Ptr);
        return new Transaction(new TxnHandle(Handle, tx));
    }

    /// <summary>
    /// Closes gracefully (<c>ns_close</c>). The native close happens once every
    /// outstanding <see cref="Transaction"/> and <see cref="Plan"/> on this
    /// connection has been disposed; the native call itself blocks while a
    /// transaction is still open.
    /// </summary>
    public void Close() => Dispose();

    /// <summary>
    /// Closes harshly (<c>ns_crash</c>). Incomplete transactions are rolled back
    /// on the next open. Outstanding transactions and plans delay the close the
    /// same way as with <see cref="Close"/>.
    /// </summary>
    public void Crash()
    {
        if (IsDisposed) return;
        Handle.CrashOnRelease = true;
        Dispose();
    }

    /// <summary>True once <see cref="Close"/>, <see cref="Crash"/> or <see cref="Dispose"/> was called.</summary>
    public bool IsDisposed => Volatile.Read(ref _disposed) != 0;

    private int _disposed;

    public void Dispose()
    {
        if (Interlocked.Exchange(ref _disposed, 1) != 0) return;
        Handle.Dispose();
        GC.SuppressFinalize(this);
    }

    /// <summary>
    /// Leases the database handle for a call made through this connection.
    /// SafeHandle alone would still allow this after Dispose() while plans or
    /// transactions keep the native handle alive, so check explicitly.
    /// </summary>
    internal Lease LeaseDb()
    {
        if (IsDisposed)
            throw new ObjectDisposedException(GetType().Name);
        return new Lease(Handle);
    }

    /// <summary>Leases a caller-supplied transaction after checking it belongs here.</summary>
    internal Lease LeaseTx(Transaction? tx) => LeaseTx(Handle, tx);

    internal static Lease LeaseTx(DbHandle owner, Transaction? tx)
    {
        if (tx is not null && !ReferenceEquals(tx.Handle.Db, owner))
            throw new ArgumentException("The transaction belongs to a different connection.", nameof(tx));
        if (tx is not null && tx.Handle.IsFinished)
            throw new InvalidOperationException("The transaction has already completed.");
        return new Lease(tx?.Handle);
    }
}
