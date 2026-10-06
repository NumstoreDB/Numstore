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

/**
 * A smart file ({@code nsdb_t*} from {@code ns_smfile_open}): a transactional
 * byte array supporting insert/remove anywhere and strided element access.
 *
 * <p>Strided methods mirror the C signatures: {@code size} is the element
 * size in bytes, {@code byteOffset} the starting byte offset, {@code stride}
 * the step in elements (1 = contiguous) and {@code nelem} the element count.
 * The Java buffer must hold at least {@code size * nelem} bytes.
 *
 * <p>Passing {@code null} for a {@link Txn} passes NULL through to numstore.
 */
public final class SmartFile extends Store {
  /** {@code SMF_END}: use as a byte offset to mean "end of file". */
  public static final long END = Long.MAX_VALUE;

  private SmartFile(long handle, String path) {
    super(handle, path);
  }

  /** {@code ns_smfile_open}. */
  public static SmartFile open(String path) {
    long h = NativeLib.smfileOpen(NativeLib.cstr(path));
    if (h == 0) {
      throw new NumstoreException("ns_smfile_open failed for '" + path + "'");
    }
    return new SmartFile(h, path);
  }

  /** {@code ns_smfile_size}: size in bytes as seen by {@code txn}. */
  public long size(Txn txn) {
    return NativeLib.smfSize(handle(), txn(txn));
  }

  // ------------------------------------------------------------ insert

  /** {@code ns_smfile_insert} of the whole array at {@code byteOffset}. */
  public long insert(Txn txn, long byteOffset, byte[] src) {
    return insert(txn, byteOffset, src, 0, src.length);
  }

  public long insert(Txn txn, long byteOffset, byte[] src, int off, int len) {
    NativeLib.Region r = NativeLib.Region.of(src, off, len);
    return NativeLib.smfInsert(handle(), txn(txn), r.buf, r.off, r.len, byteOffset);
  }

  /** Inserts the buffer's remaining bytes; position is unchanged. */
  public long insert(Txn txn, long byteOffset, ByteBuffer src) {
    NativeLib.Region r = NativeLib.Region.source(src);
    return NativeLib.smfInsert(handle(), txn(txn), r.buf, r.off, r.len, byteOffset);
  }

  // ------------------------------------------------------------ write

  /** Contiguous byte overwrite: {@code ns_smfile_write(size=1, stride=1, nelem=src.length)}. */
  public long write(Txn txn, long byteOffset, byte[] src) {
    return write(txn, src, 1, byteOffset, 1, src.length);
  }

  /** {@code ns_smfile_write} reading elements from the start of {@code src}. */
  public long write(Txn txn, byte[] src, int size, long byteOffset, long stride, long nelem) {
    NativeLib.Region r = NativeLib.Region.of(src, 0, src.length).requireElems(size, nelem);
    return NativeLib.smfWrite(handle(), txn(txn), r.buf, r.off, size, byteOffset, stride, nelem);
  }

  /** {@code ns_smfile_write} reading elements from the buffer's position. */
  public long write(Txn txn, ByteBuffer src, int size, long byteOffset, long stride, long nelem) {
    NativeLib.Region r = NativeLib.Region.source(src).requireElems(size, nelem);
    return NativeLib.smfWrite(handle(), txn(txn), r.buf, r.off, size, byteOffset, stride, nelem);
  }

  // ------------------------------------------------------------ read

  /** Contiguous byte read filling {@code dest}. */
  public long read(Txn txn, long byteOffset, byte[] dest) {
    return read(txn, dest, 1, byteOffset, 1, dest.length);
  }

  /** {@code ns_smfile_read} writing elements to the start of {@code dest}. */
  public long read(Txn txn, byte[] dest, int size, long byteOffset, long stride, long nelem) {
    NativeLib.Region r = NativeLib.Region.of(dest, 0, dest.length).requireElems(size, nelem);
    return NativeLib.smfRead(handle(), txn(txn), r.buf, r.off, size, byteOffset, stride, nelem);
  }

  /** {@code ns_smfile_read} writing elements at the buffer's position. */
  public long read(Txn txn, ByteBuffer dest, int size, long byteOffset, long stride, long nelem) {
    NativeLib.Region r = NativeLib.Region.dest(dest).requireElems(size, nelem);
    return NativeLib.smfRead(handle(), txn(txn), r.buf, r.off, size, byteOffset, stride, nelem);
  }

  // ------------------------------------------------------------ remove

  /** {@code ns_smfile_remove} discarding the removed data (dest = NULL). */
  public long remove(Txn txn, int size, long byteOffset, long stride, long nelem) {
    if (size <= 0 || nelem < 0) {
      throw new IllegalArgumentException("size must be > 0 and nelem >= 0");
    }
    return NativeLib.smfRemove(handle(), txn(txn), null, 0, size, byteOffset, stride, nelem);
  }

  /** {@code ns_smfile_remove} copying removed elements to the start of {@code dest}. */
  public long remove(Txn txn, byte[] dest, int size, long byteOffset, long stride, long nelem) {
    NativeLib.Region r = NativeLib.Region.of(dest, 0, dest.length).requireElems(size, nelem);
    return NativeLib.smfRemove(handle(), txn(txn), r.buf, r.off, size, byteOffset, stride, nelem);
  }

  /** {@code ns_smfile_remove} copying removed elements to the buffer's position. */
  public long remove(Txn txn, ByteBuffer dest, int size, long byteOffset, long stride, long nelem) {
    NativeLib.Region r = NativeLib.Region.dest(dest).requireElems(size, nelem);
    return NativeLib.smfRemove(handle(), txn(txn), r.buf, r.off, size, byteOffset, stride, nelem);
  }
}
