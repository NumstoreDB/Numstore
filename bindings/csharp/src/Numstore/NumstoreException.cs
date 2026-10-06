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

/// <summary>Raised when a numstore call reports failure.</summary>
public class NumstoreException : Exception
{
    /// <summary>The native return code, when the call returned one.</summary>
    public long? Code { get; }

    /// <summary>The native operation that failed, e.g. <c>ns_read</c>.</summary>
    public string Operation { get; }

    public NumstoreException(string operation, string message, long? code = null)
        : base(code is null ? $"{operation}: {message}" : $"{operation} ({code}): {message}")
    {
        Operation = operation;
        Code = code;
    }
}

internal static class Errors
{
    internal static string FromDb(nint db)
    {
        if (db == 0) return "unknown error";
        return Marshal.PtrToStringUTF8(Native.ns_strerror(db)) is { Length: > 0 } s ? s : "unknown error";
    }

    internal static string FromPlan(nint plan)
    {
        if (plan == 0) return "unknown error";
        return Marshal.PtrToStringUTF8(Native.ns_plan_strerror(plan)) is { Length: > 0 } s ? s : "unknown error";
    }

    internal static NumstoreException Db(string op, nint db, long? code = null) =>
        new(op, FromDb(db), code);

    internal static NumstoreException Plan(string op, nint plan, long? code = null) =>
        new(op, FromPlan(plan), code);

    internal static int Check(int rc, string op, nint db) =>
        rc < 0 ? throw Db(op, db, rc) : rc;

    internal static long Check(long rc, string op, nint db) =>
        rc < 0 ? throw Db(op, db, rc) : rc;

    internal static int CheckPlan(int rc, string op, nint plan) =>
        rc < 0 ? throw Plan(op, plan, rc) : rc;

    internal static long CheckPlan(long rc, string op, nint plan) =>
        rc < 0 ? throw Plan(op, plan, rc) : rc;
}
