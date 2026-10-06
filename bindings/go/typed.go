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

package numstore

import "unsafe"

// Elem is any fixed-size numeric type that can be passed to numstore as raw
// bytes in host byte order.
type Elem interface {
	~int8 | ~uint8 | ~int16 | ~uint16 | ~int32 | ~uint32 | ~int64 | ~uint64 |
		~float32 | ~float64 | ~complex64 | ~complex128
}

// AsBytes views s as raw bytes without copying.
func AsBytes[T Elem](s []T) []byte {
	if len(s) == 0 {
		return nil
	}
	var z T
	return unsafe.Slice((*byte)(unsafe.Pointer(unsafe.SliceData(s))), len(s)*int(unsafe.Sizeof(z)))
}

// FromBytes copies b into a new []T. Trailing bytes that do not form a whole
// element are dropped.
func FromBytes[T Elem](b []byte) []T {
	var z T
	sz := int(unsafe.Sizeof(z))
	out := make([]T, len(b)/sz)
	copy(AsBytes(out), b)
	return out
}

func sizeOf[T Elem]() uint32 {
	var z T
	return uint32(unsafe.Sizeof(z))
}

// ReadInto runs a READ/REMOVE query into a typed slice and returns the number
// of bytes read.
//
//	dst := make([]uint32, 10)
//	_, err := numstore.ReadInto(db, tx, dst, "read foo[0:10]")
func ReadInto[T Elem](db *DB, tx *Txn, dst []T, query string) (int64, error) {
	return db.Read(tx, AsBytes(dst), query)
}

// WriteFrom runs an INSERT/WRITE query from a typed slice.
//
//	_, err := numstore.WriteFrom(db, tx, []uint32{1, 2, 3}, "insert foo 0 3")
func WriteFrom[T Elem](db *DB, tx *Txn, src []T, query string) (int64, error) {
	return db.Write(tx, AsBytes(src), query)
}

// ReadAllAs runs a READ/REMOVE query and returns the result as a []T.
func ReadAllAs[T Elem](db *DB, tx *Txn, query string) ([]T, error) {
	b, err := db.ReadAll(tx, query)
	if err != nil {
		return nil, err
	}
	return FromBytes[T](b), nil
}

// SmartWrite is SmartFile.Write with the element size taken from T.
func SmartWrite[T Elem](f *SmartFile, tx *Txn, src []T, offset, stride int64) (int64, error) {
	return f.Write(tx, AsBytes(src), sizeOf[T](), offset, stride)
}

// SmartRead is SmartFile.Read with the element size taken from T.
func SmartRead[T Elem](f *SmartFile, tx *Txn, dst []T, offset, stride int64) (int64, error) {
	return f.Read(tx, AsBytes(dst), sizeOf[T](), offset, stride)
}

// SmartRemove removes len(dst) elements of type T into dst.
func SmartRemove[T Elem](f *SmartFile, tx *Txn, dst []T, offset, stride int64) (int64, error) {
	return f.Remove(tx, AsBytes(dst), sizeOf[T](), offset, stride, uint64(len(dst)))
}
