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

namespace Numstore.Interop;

/// <summary>
/// Raw, 1:1 P/Invoke declarations for <c>numstore.h</c>.
///
/// Type mapping (default NS_TYPE_ALIASES):
/// <list type="bullet">
///   <item><c>t_size</c>  → <see cref="uint"/></item>
///   <item><c>b_size</c>  → <see cref="ulong"/></item>
///   <item><c>sb_size</c> → <see cref="long"/></item>
///   <item>opaque pointers (<c>nsdb_t*</c>, <c>txn_t*</c>, …) → <see cref="nint"/></item>
/// </list>
///
/// <para>
/// <b>Variadic functions.</b> .NET cannot call C varargs directly. Every
/// variadic entry point is bound here with a fixed signature that always passes
/// the format <c>"%s"</c> followed by the full query string, so the query is
/// never interpreted as a format string. That fixed signature is ABI-identical
/// to the variadic call on x86/x64 (Windows and SysV) and on Linux/Windows
/// ARM64. Apple ARM64 is the exception: variadic arguments go on the stack, not
/// in registers. For that platform each function has an <c>*_AppleArm64</c>
/// twin that fills the 8 argument registers (x0–x7) with padding so the query
/// pointer lands in the first stack slot, which is exactly where the callee's
/// <c>va_arg</c> looks. Use the dispatching helpers in <see cref="VarArgs"/>
/// rather than calling these directly.
/// </para>
/// </summary>
public static unsafe partial class Native
{
    /// <summary>
    /// Base name of the native library. The runtime probes
    /// <c>libnumstore.so</c>, <c>libnumstore.dylib</c> or <c>numstore.dll</c>.
    /// Override resolution with <see cref="NativeLibrary.SetDllImportResolver"/>
    /// if the library lives elsewhere.
    /// </summary>
    public const string Lib = "numstore";

    /// <summary>The format string every variadic call is given.</summary>
    internal const string PassThroughFormat = "%s";

    //////////////////////////////////// Lifecycle

    [LibraryImport(Lib, StringMarshalling = StringMarshalling.Utf8)]
    public static partial nint ns_open(string path);

    [LibraryImport(Lib, StringMarshalling = StringMarshalling.Utf8)]
    public static partial nint ns_smfile_open(string path);

    [LibraryImport(Lib, StringMarshalling = StringMarshalling.Utf8)]
    public static partial int ns_cleanup(string path);

    [LibraryImport(Lib)]
    public static partial int ns_close(nint ns);

    [LibraryImport(Lib)]
    public static partial int ns_crash(nint ns);

    //////////////////////////////////// Variable getters

    [LibraryImport(Lib)]
    public static partial ulong ns_var_len(nint var);

    [LibraryImport(Lib)]
    public static partial void ns_var_free(nint var);

    //////////////////////////////////// Error handling

    /// <returns>A borrowed <c>const char*</c>; do not free.</returns>
    [LibraryImport(Lib)]
    public static partial nint ns_strerror(nint ns);

    /// <returns>A borrowed <c>const char*</c>; do not free.</returns>
    [LibraryImport(Lib)]
    public static partial nint ns_plan_strerror(nint plan);

    //////////////////////////////////// Transactions

    [LibraryImport(Lib)]
    public static partial nint ns_begin(nint ns);

    [LibraryImport(Lib)]
    public static partial int ns_commit(nint ns, nint txn);

    [LibraryImport(Lib)]
    public static partial int ns_rollback(nint ns, nint txn);

    //////////////////////////////////// Plans (non-variadic half)

    [LibraryImport(Lib)]
    public static partial void ns_plan_free(nint plan);

    [LibraryImport(Lib)]
    public static partial int ns_plan_execute(nint plan, nint tx);

    [LibraryImport(Lib)]
    public static partial nint ns_plan_get_var(nint plan, nint tx);

    [LibraryImport(Lib)]
    public static partial long ns_plan_read(nint plan, nint txn, void* dest, ulong dlen);

    [LibraryImport(Lib)]
    public static partial long ns_plan_write(nint plan, nint tx, void* src, ulong dlen);

    [LibraryImport(Lib)]
    public static partial void* ns_plan_malloc(nint plan, nint tx, ulong* dlen);

    //////////////////////////////////// Variadic: standard ABI form
    // C: f(fixed..., const char *fmt, ...)  called as  f(fixed..., "%s", query)

    [LibraryImport(Lib, EntryPoint = "ns_plan_fcreate", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint ns_plan_fcreate_std(nint db, string fmt, string query);

    [LibraryImport(Lib, EntryPoint = "ns_execute", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial int ns_execute_std(nint db, nint tx, string fmt, string query);

    [LibraryImport(Lib, EntryPoint = "ns_get_var", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint ns_get_var_std(nint db, nint tx, string fmt, string query);

    [LibraryImport(Lib, EntryPoint = "ns_read", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial long ns_read_std(nint db, nint txn, void* dest, ulong dlen, string fmt, string query);

    [LibraryImport(Lib, EntryPoint = "ns_write", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial long ns_write_std(nint db, nint txn, void* src, ulong dlen, string fmt, string query);

    [LibraryImport(Lib, EntryPoint = "ns_malloc", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void* ns_malloc_std(nint db, nint txn, ulong* dlen, string fmt, string query);

    //////////////////////////////////// Variadic: Apple ARM64 form
    // x0..x7 are filled (fixed args + nint padding) so the variadic query pointer
    // is passed in the first stack slot.

    [LibraryImport(Lib, EntryPoint = "ns_plan_fcreate", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint ns_plan_fcreate_AppleArm64(
        nint db, string fmt,
        nint p2, nint p3, nint p4, nint p5, nint p6, nint p7,
        string query);

    [LibraryImport(Lib, EntryPoint = "ns_execute", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial int ns_execute_AppleArm64(
        nint db, nint tx, string fmt,
        nint p3, nint p4, nint p5, nint p6, nint p7,
        string query);

    [LibraryImport(Lib, EntryPoint = "ns_get_var", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial nint ns_get_var_AppleArm64(
        nint db, nint tx, string fmt,
        nint p3, nint p4, nint p5, nint p6, nint p7,
        string query);

    [LibraryImport(Lib, EntryPoint = "ns_read", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial long ns_read_AppleArm64(
        nint db, nint txn, void* dest, ulong dlen, string fmt,
        nint p5, nint p6, nint p7,
        string query);

    [LibraryImport(Lib, EntryPoint = "ns_write", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial long ns_write_AppleArm64(
        nint db, nint txn, void* src, ulong dlen, string fmt,
        nint p5, nint p6, nint p7,
        string query);

    [LibraryImport(Lib, EntryPoint = "ns_malloc", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial void* ns_malloc_AppleArm64(
        nint db, nint txn, ulong* dlen, string fmt,
        nint p4, nint p5, nint p6, nint p7,
        string query);

    //////////////////////////////////// Smart files

    [LibraryImport(Lib)]
    public static partial long ns_smfile_size(nint smf, nint tx);

    [LibraryImport(Lib)]
    public static partial long ns_smfile_insert(nint smf, nint tx, void* src, long bofst, ulong slen);

    [LibraryImport(Lib)]
    public static partial long ns_smfile_write(
        nint smf, nint tx, void* src, uint size, long bofst, long stride, ulong nelem);

    [LibraryImport(Lib)]
    public static partial long ns_smfile_read(
        nint smf, nint tx, void* dest, uint size, long bofst, long stride, ulong nelem);

    [LibraryImport(Lib)]
    public static partial long ns_smfile_remove(
        nint smf, nint tx, void* dest, uint size, long bofst, long stride, ulong nelem);
}

/// <summary>
/// Calls the variadic numstore functions with the correct calling convention
/// for the current platform. Each takes the complete query string; it is passed
/// through a <c>"%s"</c> format, so <c>%</c> characters in it are literal.
/// </summary>
public static unsafe class VarArgs
{
    /// <summary>True when variadic args must be passed on the stack (Apple ARM64).</summary>
    public static readonly bool UseAppleArm64Convention =
        OperatingSystem.IsMacOS() || OperatingSystem.IsIOS() ||
        OperatingSystem.IsTvOS() || OperatingSystem.IsMacCatalyst()
            ? RuntimeInformation.ProcessArchitecture == Architecture.Arm64
            : false;

    private const string F = Native.PassThroughFormat;

    public static nint ns_plan_fcreate(nint db, string query) =>
        UseAppleArm64Convention
            ? Native.ns_plan_fcreate_AppleArm64(db, F, 0, 0, 0, 0, 0, 0, query)
            : Native.ns_plan_fcreate_std(db, F, query);

    public static int ns_execute(nint db, nint tx, string query) =>
        UseAppleArm64Convention
            ? Native.ns_execute_AppleArm64(db, tx, F, 0, 0, 0, 0, 0, query)
            : Native.ns_execute_std(db, tx, F, query);

    public static nint ns_get_var(nint db, nint tx, string query) =>
        UseAppleArm64Convention
            ? Native.ns_get_var_AppleArm64(db, tx, F, 0, 0, 0, 0, 0, query)
            : Native.ns_get_var_std(db, tx, F, query);

    public static long ns_read(nint db, nint txn, void* dest, ulong dlen, string query) =>
        UseAppleArm64Convention
            ? Native.ns_read_AppleArm64(db, txn, dest, dlen, F, 0, 0, 0, query)
            : Native.ns_read_std(db, txn, dest, dlen, F, query);

    public static long ns_write(nint db, nint txn, void* src, ulong dlen, string query) =>
        UseAppleArm64Convention
            ? Native.ns_write_AppleArm64(db, txn, src, dlen, F, 0, 0, 0, query)
            : Native.ns_write_std(db, txn, src, dlen, F, query);

    public static void* ns_malloc(nint db, nint txn, ulong* dlen, string query) =>
        UseAppleArm64Convention
            ? Native.ns_malloc_AppleArm64(db, txn, dlen, F, 0, 0, 0, 0, query)
            : Native.ns_malloc_std(db, txn, dlen, F, query);
}
