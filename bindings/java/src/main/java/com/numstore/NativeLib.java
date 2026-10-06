/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.

package com.numstore;

import java.nio.ByteBuffer;
import java.nio.ReadOnlyBufferException;
import java.nio.charset.StandardCharsets;

/**
 * Raw JNI entry points. Everything here is package-private; the public API
 * lives in {@link Numstore}, {@link SmartFile}, {@link Txn}, {@link Plan} and
 * {@link Var}.
 *
 * <p>Conventions:
 * <ul>
 *   <li>Native pointers travel as {@code long}; 0 means NULL.</li>
 *   <li>Strings travel as NUL-terminated UTF-8 {@code byte[]} (see {@link #cstr}).
 *       Queries are always passed to C as {@code fmt = "%s"} so a {@code %} in a
 *       query is never interpreted as a format directive.</li>
 *   <li>Buffers travel as {@code (Object buf, int off, int len)} where {@code buf}
 *       is either a {@code byte[]} or a direct {@link ByteBuffer}. Direct
 *       buffers are zero-copy; arrays are copied through a native scratch
 *       buffer (never pinned, since numstore calls can block).</li>
 *   <li>On failure, natives throw {@link NumstoreException} carrying the
 *       numstore error string.</li>
 * </ul>
 *
 * <p>The library is loaded from {@code -Dnumstore.jni.library=/abs/path/libnumstore_jni.so}
 * if set, otherwise via {@code System.loadLibrary("numstore_jni")}.
 */
final class NativeLib {
  private NativeLib() {}

  static {
    String path = System.getProperty("numstore.jni.library");
    if (path != null && !path.isEmpty()) {
      System.load(path);
    } else {
      System.loadLibrary("numstore_jni");
    }
  }

  // ------------------------------------------------------------ Lifecycle

  /** Returns 0 on failure (no handle exists to fetch an error string from). */
  static native long open(byte[] path);

  /** Returns 0 on failure. */
  static native long smfileOpen(byte[] path);

  /** Returns the raw ns_cleanup result. */
  static native int cleanup(byte[] path);

  static native void close(long db);

  static native void crash(long db);

  // ------------------------------------------------------------ Variables

  static native long varLen(long var);

  static native void varFree(long var);

  // ------------------------------------------------------------ Transactions

  static native long begin(long db);

  static native void commit(long db, long txn);

  static native void rollback(long db, long txn);

  // ------------------------------------------------------------ Plans

  static native long planCreate(long db, byte[] query);

  static native void planFree(long plan);

  // ------------------------------------------------------------ Queries
  // When plan != 0 the ns_plan_* variant is called and db/query are ignored.

  static native void execute(long db, long plan, long txn, byte[] query);

  static native long getVar(long db, long plan, long txn, byte[] query);

  static native long read(long db, long plan, long txn, Object buf, int off, int len, byte[] query);

  static native long write(long db, long plan, long txn, Object buf, int off, int len, byte[] query);

  static native byte[] mallocRead(long db, long plan, long txn, byte[] query);

  // ------------------------------------------------------------ Smart files

  static native long smfSize(long smf, long txn);

  static native long smfInsert(long smf, long txn, Object buf, int off, int len, long bofst);

  static native long smfWrite(
      long smf, long txn, Object buf, int off, int size, long bofst, long stride, long nelem);

  static native long smfRead(
      long smf, long txn, Object buf, int off, int size, long bofst, long stride, long nelem);

  /** {@code buf} may be null, in which case removed data is discarded. */
  static native long smfRemove(
      long smf, long txn, Object buf, int off, int size, long bofst, long stride, long nelem);

  // ------------------------------------------------------------ Helpers

  static byte[] cstr(String s) {
    if (s == null) {
      throw new NullPointerException();
    }
    byte[] utf8 = s.getBytes(StandardCharsets.UTF_8);
    for (byte b : utf8) {
      if (b == 0) {
        throw new IllegalArgumentException("string contains a NUL character");
      }
    }
    byte[] out = new byte[utf8.length + 1];
    System.arraycopy(utf8, 0, out, 0, utf8.length);
    return out;
  }

  /** A (buffer, offset, length) triple ready to hand to a native method. */
  static final class Region {
    final Object buf;
    final int off;
    final int len;

    Region(Object buf, int off, int len) {
      this.buf = buf;
      this.off = off;
      this.len = len;
    }

    static Region of(byte[] a, int off, int len) {
      if (a == null) {
        throw new NullPointerException();
      }
      if (off < 0 || len < 0 || off > a.length - len) {
        throw new IndexOutOfBoundsException(
            "off=" + off + " len=" + len + " array length=" + a.length);
      }
      return new Region(a, off, len);
    }

    /** Region numstore will read from (source). Position is not modified. */
    static Region source(ByteBuffer b) {
      if (b.isDirect()) {
        return new Region(b, b.position(), b.remaining());
      }
      if (b.hasArray()) {
        return new Region(b.array(), b.arrayOffset() + b.position(), b.remaining());
      }
      // Read-only heap buffer: no accessible array, so take a copy.
      byte[] tmp = new byte[b.remaining()];
      b.duplicate().get(tmp);
      return new Region(tmp, 0, tmp.length);
    }

    /** Region numstore will write into (destination). Position is not modified. */
    static Region dest(ByteBuffer b) {
      if (b.isReadOnly()) {
        throw new ReadOnlyBufferException();
      }
      if (b.isDirect()) {
        return new Region(b, b.position(), b.remaining());
      }
      return new Region(b.array(), b.arrayOffset() + b.position(), b.remaining());
    }

    /** Ensures the region can hold {@code size * nelem} bytes. */
    Region requireElems(int size, long nelem) {
      if (size <= 0) {
        throw new IllegalArgumentException("element size must be > 0, got " + size);
      }
      if (nelem < 0) {
        throw new IllegalArgumentException("nelem must be >= 0, got " + nelem);
      }
      if (nelem > len / size) {
        throw new IndexOutOfBoundsException(
            nelem + " elements of " + size + " bytes do not fit in " + len + " bytes");
      }
      return this;
    }
  }
}
