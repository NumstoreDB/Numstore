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
 * A variable ({@code nsdb_var_t*}). Variables own their memory independently
 * of the database, so they may be closed before or after the database.
 */
public final class Var implements AutoCloseable {
  private final AtomicLong handle;

  Var(long handle) {
    this.handle = new AtomicLong(handle);
  }

  /** Length in elements, not bytes ({@code ns_var_len}). */
  public long length() {
    long h = handle.get();
    if (h == 0) {
      throw new IllegalStateException("variable is closed");
    }
    return NativeLib.varLen(h);
  }

  /** {@code ns_var_free}. Idempotent. */
  @Override
  public void close() {
    long h = handle.getAndSet(0);
    if (h != 0) {
      NativeLib.varFree(h);
    }
  }
}
