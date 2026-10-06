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
import java.util.concurrent.atomic.AtomicLong;

/**
 * A prepared query plan ({@code nsdb_plan_t*}), created with
 * {@link Numstore#plan(String)}.
 *
 * <p>Plans must be freed before their database is closed; any plan still open
 * when {@link Numstore#close()} runs is closed automatically first.
 */
public final class Plan implements AutoCloseable {
  private final Numstore db;
  private final String query;
  private final AtomicLong handle;

  Plan(Numstore db, String query, long handle) {
    this.db = db;
    this.query = query;
    this.handle = new AtomicLong(handle);
  }

  public String query() {
    return query;
  }

  /** {@code ns_plan_execute}: for queries that take no data. */
  public void execute(Txn txn) {
    NativeLib.execute(0, handle(), db.txn(txn), null);
  }

  /** {@code ns_plan_get_var}. Close the returned {@link Var} when done. */
  public Var getVar(Txn txn) {
    return new Var(NativeLib.getVar(0, handle(), db.txn(txn), null));
  }

  /** {@code ns_plan_read} into the whole array. READ/REMOVE queries only. */
  public long read(Txn txn, byte[] dest) {
    return read(txn, dest, 0, dest.length);
  }

  /** {@code ns_plan_read} into {@code dest[off, off+len)}. */
  public long read(Txn txn, byte[] dest, int off, int len) {
    return read(txn, NativeLib.Region.of(dest, off, len));
  }

  /** {@code ns_plan_read} into the buffer's remaining bytes; position is unchanged. */
  public long read(Txn txn, ByteBuffer dest) {
    return read(txn, NativeLib.Region.dest(dest));
  }

  /** {@code ns_plan_write} from the whole array. INSERT/WRITE queries only. */
  public long write(Txn txn, byte[] src) {
    return write(txn, src, 0, src.length);
  }

  /** {@code ns_plan_write} from {@code src[off, off+len)}. */
  public long write(Txn txn, byte[] src, int off, int len) {
    return write(txn, NativeLib.Region.of(src, off, len));
  }

  /** {@code ns_plan_write} from the buffer's remaining bytes; position is unchanged. */
  public long write(Txn txn, ByteBuffer src) {
    return write(txn, NativeLib.Region.source(src));
  }

  /** {@code ns_plan_malloc}: reads the full result into a new array. */
  public byte[] readAll(Txn txn) {
    return NativeLib.mallocRead(0, handle(), db.txn(txn), null);
  }

  /** {@code ns_plan_free}. Idempotent. */
  @Override
  public void close() {
    long h = handle.getAndSet(0);
    if (h != 0) {
      db.forget(this);
      NativeLib.planFree(h);
    }
  }

  private long read(Txn txn, NativeLib.Region r) {
    return NativeLib.read(0, handle(), db.txn(txn), r.buf, r.off, r.len, null);
  }

  private long write(Txn txn, NativeLib.Region r) {
    return NativeLib.write(0, handle(), db.txn(txn), r.buf, r.off, r.len, null);
  }

  private long handle() {
    long h = handle.get();
    if (h == 0) {
      throw new IllegalStateException("plan is closed: " + query);
    }
    return h;
  }

  @Override
  public String toString() {
    return "Plan[" + query + "]";
  }
}
