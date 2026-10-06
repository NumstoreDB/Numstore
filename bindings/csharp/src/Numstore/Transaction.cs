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
using Numstore.Interop;

namespace Numstore;

/// <summary>
/// A numstore transaction (<c>txn_t*</c>). Disposing a transaction that was not
/// committed rolls it back, so the usual pattern is:
/// <code>
/// using var tx = db.Begin();
/// db.Execute(tx, "create foo u32");
/// tx.Commit();
/// </code>
/// </summary>
public sealed class Transaction : IDisposable
{
    internal TxnHandle Handle { get; }

    internal Transaction(TxnHandle handle) => Handle = handle;

    /// <summary>True once committed, rolled back or disposed.</summary>
    public bool IsCompleted => Handle.IsFinished || Handle.IsClosed;

    /// <summary>Commits the transaction (<c>ns_commit</c>).</summary>
    public void Commit() => End(commit: true);

    /// <summary>Rolls the transaction back (<c>ns_rollback</c>).</summary>
    public void Rollback() => End(commit: false);

    private void End(bool commit)
    {
        if (IsCompleted)
            throw new InvalidOperationException("The transaction has already completed.");

        using (var db = new Lease(Handle.Db))
        using (var tx = new Lease(Handle))
        {
            int rc = commit ? Native.ns_commit(db.Ptr, tx.Ptr) : Native.ns_rollback(db.Ptr, tx.Ptr);
            // On failure the transaction is assumed to still be open; leaving it
            // unfinished means Dispose() will attempt a rollback.
            Errors.Check(rc, commit ? "ns_commit" : "ns_rollback", db.Ptr);
            Handle.MarkFinished();
        }
        Handle.Dispose();
    }

    /// <summary>Rolls back if still open, then releases the transaction.</summary>
    public void Dispose() => Handle.Dispose();
}
