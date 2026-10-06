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
 * Common base for anything backed by an {@code nsdb_t*}: a {@link Numstore}
 * database or a {@link SmartFile}.
 *
 * <p>Closing is idempotent. Closing a store while another thread is still
 * calling into it is a caller error (the native handle is freed underneath
 * that call), exactly as in C. Note that, as in C, {@link #close()} blocks
 * while a transaction is still open.
 */
public abstract class Store implements AutoCloseable {
  private final AtomicLong handle;
  private final String path;

  Store(long handle, String path) {
    this.handle = new AtomicLong(handle);
    this.path = path;
  }

  /** Removes the numstore database or smart file at {@code path}. */
  public static void cleanup(String path) {
    if (NativeLib.cleanup(NativeLib.cstr(path)) < 0) {
      throw new NumstoreException("ns_cleanup failed for '" + path + "'");
    }
  }

  public final String path() {
    return path;
  }

  public final boolean isOpen() {
    return handle.get() != 0;
  }

  /** Begins a transaction ({@code ns_begin}). */
  public Txn begin() {
    return new Txn(this, NativeLib.begin(handle()));
  }

  /** Graceful close ({@code ns_close}). Blocks while a transaction is open. */
  @Override
  public void close() {
    long h = handle.getAndSet(0);
    if (h != 0) {
      beforeNativeClose();
      NativeLib.close(h);
    }
  }

  /**
   * Harsh close ({@code ns_crash}). Incomplete transactions are rolled back on
   * the next open.
   */
  public void crash() {
    long h = handle.getAndSet(0);
    if (h != 0) {
      beforeNativeClose();
      NativeLib.crash(h);
    }
  }

  /** Hook for subclasses to release dependents (e.g. plans) first. */
  void beforeNativeClose() {}

  final long handle() {
    long h = handle.get();
    if (h == 0) {
      throw new IllegalStateException(getClass().getSimpleName() + " '" + path + "' is closed");
    }
    return h;
  }

  /** Native txn pointer for a call on this store; {@code null} maps to NULL. */
  final long txn(Txn txn) {
    if (txn == null) {
      return 0;
    }
    if (txn.store() != this) {
      throw new IllegalArgumentException("transaction belongs to a different store");
    }
    return txn.handle();
  }

  @Override
  public String toString() {
    return getClass().getSimpleName() + "[" + path + (isOpen() ? "" : ", closed") + "]";
  }
}
