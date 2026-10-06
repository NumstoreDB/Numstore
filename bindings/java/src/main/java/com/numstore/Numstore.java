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
import java.util.ArrayList;
import java.util.Collections;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * A numstore database ({@code nsdb_t*} from {@code ns_open}).
 *
 * <p>Queries are plain strings. The C API's printf-style formatting is not
 * exposed; build the string in Java (e.g. {@code String.format}) instead.
 * Passing {@code null} for a {@link Txn} passes NULL through to numstore.
 *
 * <pre>{@code
 * try (Numstore db = Numstore.open("data.ns");
 *      Txn tx = db.begin()) {
 *   db.execute(tx, "create foo u32");
 *   ByteBuffer src = ByteBuffer.allocateDirect(40).order(ByteOrder.nativeOrder());
 *   for (int i = 0; i < 10; i++) src.putInt(i);
 *   src.flip();
 *   db.write(tx, src, "insert foo 0 10");
 *   tx.commit();
 * }
 * }</pre>
 *
 * Multi-byte values are exchanged in raw native byte order; use
 * {@code ByteOrder.nativeOrder()} on your buffers.
 */
public final class Numstore extends Store {
  private final Set<Plan> plans = Collections.newSetFromMap(new ConcurrentHashMap<>());

  private Numstore(long handle, String path) {
    super(handle, path);
  }

  /** {@code ns_open}. */
  public static Numstore open(String path) {
    long h = NativeLib.open(NativeLib.cstr(path));
    if (h == 0) {
      throw new NumstoreException("ns_open failed for '" + path + "'");
    }
    return new Numstore(h, path);
  }

  // ------------------------------------------------------------ Plans

  /** {@code ns_plan_fcreate}. Close the plan (or the database) when done. */
  public Plan plan(String query) {
    long h = NativeLib.planCreate(handle(), NativeLib.cstr(query));
    Plan p = new Plan(this, query, h);
    plans.add(p);
    return p;
  }

  void forget(Plan p) {
    plans.remove(p);
  }

  @Override
  void beforeNativeClose() {
    for (Plan p : new ArrayList<>(plans)) {
      p.close();
    }
  }

  // ------------------------------------------------------------ Queries

  /** {@code ns_execute}: for queries that take no data (e.g. {@code create}, {@code delete}). */
  public void execute(Txn txn, String query) {
    NativeLib.execute(handle(), 0, txn(txn), NativeLib.cstr(query));
  }

  /** {@code ns_get_var}. Close the returned {@link Var} when done. */
  public Var getVar(Txn txn, String query) {
    return new Var(NativeLib.getVar(handle(), 0, txn(txn), NativeLib.cstr(query)));
  }

  /** {@code ns_read} into the whole array. READ/REMOVE queries only. */
  public long read(Txn txn, byte[] dest, String query) {
    return read(txn, dest, 0, dest.length, query);
  }

  /** {@code ns_read} into {@code dest[off, off+len)}. */
  public long read(Txn txn, byte[] dest, int off, int len, String query) {
    return read(txn, NativeLib.Region.of(dest, off, len), query);
  }

  /** {@code ns_read} into the buffer's remaining bytes; position is unchanged. */
  public long read(Txn txn, ByteBuffer dest, String query) {
    return read(txn, NativeLib.Region.dest(dest), query);
  }

  /** {@code ns_write} from the whole array. INSERT/WRITE queries only. */
  public long write(Txn txn, byte[] src, String query) {
    return write(txn, src, 0, src.length, query);
  }

  /** {@code ns_write} from {@code src[off, off+len)}. */
  public long write(Txn txn, byte[] src, int off, int len, String query) {
    return write(txn, NativeLib.Region.of(src, off, len), query);
  }

  /** {@code ns_write} from the buffer's remaining bytes; position is unchanged. */
  public long write(Txn txn, ByteBuffer src, String query) {
    return write(txn, NativeLib.Region.source(src), query);
  }

  /** {@code ns_malloc}: reads the full result into a new array. READ/REMOVE queries only. */
  public byte[] readAll(Txn txn, String query) {
    return NativeLib.mallocRead(handle(), 0, txn(txn), NativeLib.cstr(query));
  }

  private long read(Txn txn, NativeLib.Region r, String query) {
    return NativeLib.read(handle(), 0, txn(txn), r.buf, r.off, r.len, NativeLib.cstr(query));
  }

  private long write(Txn txn, NativeLib.Region r, String query) {
    return NativeLib.write(handle(), 0, txn(txn), r.buf, r.off, r.len, NativeLib.cstr(query));
  }
}
