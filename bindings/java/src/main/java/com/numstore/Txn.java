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

import java.util.concurrent.atomic.AtomicLong;

/**
 * A transaction ({@code txn_t*}). Use with try-with-resources: if neither
 * {@link #commit()} nor {@link #rollback()} was called, {@link #close()}
 * rolls back.
 *
 * <pre>{@code
 * try (Txn tx = db.begin()) {
 *   db.execute(tx, "create foo u32");
 *   tx.commit();
 * }
 * }</pre>
 */
public final class Txn implements AutoCloseable {
  private final Store store;
  private final AtomicLong handle;

  Txn(Store store, long handle) {
    this.store = store;
    this.handle = new AtomicLong(handle);
  }

  /** The store this transaction was started on. */
  public Store store() {
    return store;
  }

  /** True until the transaction has been committed or rolled back. */
  public boolean isActive() {
    return handle.get() != 0;
  }

  /** {@code ns_commit}. The transaction is finished afterwards, even on failure. */
  public void commit() {
    NativeLib.commit(store.handle(), take());
  }

  /** {@code ns_rollback}. The transaction is finished afterwards, even on failure. */
  public void rollback() {
    NativeLib.rollback(store.handle(), take());
  }

  /** Rolls back if still active; otherwise does nothing. */
  @Override
  public void close() {
    long h = handle.getAndSet(0);
    if (h != 0 && store.isOpen()) {
      NativeLib.rollback(store.handle(), h);
    }
  }

  long handle() {
    long h = handle.get();
    if (h == 0) {
      throw new IllegalStateException("transaction is no longer active");
    }
    return h;
  }

  private long take() {
    long h = handle.getAndSet(0);
    if (h == 0) {
      throw new IllegalStateException("transaction is no longer active");
    }
    return h;
  }
}
