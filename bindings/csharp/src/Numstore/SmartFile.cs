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
/// A numstore smart file (<c>ns_smfile_open</c>): a transactional byte array
/// with insert, strided read/write and remove.
///
/// Offsets are always in <b>bytes</b>. <c>stride</c> is in <b>elements</b> of
/// the element type <c>T</c> (1 = contiguous, 2 = every other element, …), and
/// the element size passed to numstore is <c>sizeof(T)</c>.
/// <code>
/// using var smf = SmartFile.Open("data.smf");
/// using var tx = smf.Begin();
/// smf.Insert&lt;uint&gt;(tx, new uint[] { 0, 1, 2, 3, 4, 5 }, byteOffset: 0);
/// smf.Write&lt;uint&gt;(tx, new uint[] { 6, 7, 8 }, byteOffset: 4, stride: 2); // [0,6,2,7,4,8]
/// var odd = new uint[3];
/// smf.Read&lt;uint&gt;(tx, odd, byteOffset: 4, stride: 2);                  // [6,7,8]
/// tx.Commit();
/// </code>
/// </summary>
public sealed unsafe class SmartFile : Connection
{
    private SmartFile(DbHandle h, string path) : base(h, path) { }

    /// <summary>Opens (or creates) a smart file.</summary>
    public static SmartFile Open(string path)
    {
        ArgumentNullException.ThrowIfNull(path);
        return new SmartFile(OpenHandle(Native.ns_smfile_open(path), "ns_smfile_open", path), path);
    }

    /// <summary>Size of the file in bytes as seen by <paramref name="tx"/> (<c>ns_smfile_size</c>).</summary>
    public long Size(Transaction? tx = null)
    {
        using var db = LeaseDb();
        using var t = LeaseTx(tx);
        return Errors.Check(Native.ns_smfile_size(db.Ptr, t.Ptr), "ns_smfile_size", db.Ptr);
    }

    /// <summary>
    /// Inserts <paramref name="src"/> at <paramref name="byteOffset"/>, shifting
    /// later bytes right (<c>ns_smfile_insert</c>).
    /// </summary>
    public long Insert<T>(Transaction? tx, ReadOnlySpan<T> src, long byteOffset) where T : unmanaged
    {
        ReadOnlySpan<byte> bytes = MemoryMarshal.AsBytes(src);
        using var db = LeaseDb();
        using var t = LeaseTx(tx);
        fixed (byte* s = bytes)
            return Errors.Check(
                Native.ns_smfile_insert(db.Ptr, t.Ptr, s, byteOffset, (ulong)bytes.Length),
                "ns_smfile_insert", db.Ptr);
    }

    /// <inheritdoc cref="Insert{T}(Transaction?, ReadOnlySpan{T}, long)"/>
    public long Insert<T>(Transaction? tx, T[] src, long byteOffset) where T : unmanaged =>
        Insert(tx, (ReadOnlySpan<T>)src, byteOffset);

    /// <summary>
    /// Overwrites <c>src.Length</c> elements starting at <paramref name="byteOffset"/>,
    /// stepping <paramref name="stride"/> elements between writes (<c>ns_smfile_write</c>).
    /// </summary>
    public long Write<T>(Transaction? tx, ReadOnlySpan<T> src, long byteOffset, long stride = 1) where T : unmanaged
    {
        using var db = LeaseDb();
        using var t = LeaseTx(tx);
        fixed (T* s = src)
            return Errors.Check(
                Native.ns_smfile_write(db.Ptr, t.Ptr, s, (uint)sizeof(T), byteOffset, stride, (ulong)src.Length),
                "ns_smfile_write", db.Ptr);
    }

    /// <inheritdoc cref="Write{T}(Transaction?, ReadOnlySpan{T}, long, long)"/>
    public long Write<T>(Transaction? tx, T[] src, long byteOffset, long stride = 1) where T : unmanaged =>
        Write(tx, (ReadOnlySpan<T>)src, byteOffset, stride);

    /// <summary>
    /// Reads <c>dest.Length</c> elements starting at <paramref name="byteOffset"/>,
    /// stepping <paramref name="stride"/> elements between reads (<c>ns_smfile_read</c>).
    /// </summary>
    public long Read<T>(Transaction? tx, Span<T> dest, long byteOffset, long stride = 1) where T : unmanaged
    {
        using var db = LeaseDb();
        using var t = LeaseTx(tx);
        fixed (T* d = dest)
            return Errors.Check(
                Native.ns_smfile_read(db.Ptr, t.Ptr, d, (uint)sizeof(T), byteOffset, stride, (ulong)dest.Length),
                "ns_smfile_read", db.Ptr);
    }

    /// <summary>
    /// Removes <c>dest.Length</c> elements starting at <paramref name="byteOffset"/>
    /// (stepping <paramref name="stride"/>), copying the removed values into
    /// <paramref name="dest"/> (<c>ns_smfile_remove</c>).
    /// </summary>
    public long Remove<T>(Transaction? tx, Span<T> dest, long byteOffset, long stride = 1) where T : unmanaged
    {
        using var db = LeaseDb();
        using var t = LeaseTx(tx);
        fixed (T* d = dest)
            return Errors.Check(
                Native.ns_smfile_remove(db.Ptr, t.Ptr, d, (uint)sizeof(T), byteOffset, stride, (ulong)dest.Length),
                "ns_smfile_remove", db.Ptr);
    }

    /// <summary>
    /// Removes <paramref name="count"/> elements of <typeparamref name="T"/> without
    /// keeping them (passes a NULL destination to <c>ns_smfile_remove</c>).
    /// </summary>
    public long Remove<T>(Transaction? tx, long byteOffset, ulong count, long stride = 1) where T : unmanaged
    {
        using var db = LeaseDb();
        using var t = LeaseTx(tx);
        return Errors.Check(
            Native.ns_smfile_remove(db.Ptr, t.Ptr, null, (uint)sizeof(T), byteOffset, stride, count),
            "ns_smfile_remove", db.Ptr);
    }
}
