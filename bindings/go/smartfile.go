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

/*
#include <stdlib.h>
#include "numstore.h"
*/
import "C"

import (
	"fmt"
	"math"
	"runtime"
	"unsafe"
)

// SmartFile is an open numstore smart file: a transactional byte array.
type SmartFile struct{ h *handle }

// OpenSmartFile opens (or creates) the smart file at path.
func OpenSmartFile(path string) (*SmartFile, error) {
	cp := C.CString(path)
	defer C.free(unsafe.Pointer(cp))
	p := C.ns_smfile_open(cp)
	if p == nil {
		return nil, &Error{Op: "ns_smfile_open", Msg: path}
	}
	return &SmartFile{h: &handle{p: p, plans: map[*Plan]struct{}{}}}, nil
}

// Close closes the file gracefully, blocking while a transaction is open.
func (f *SmartFile) Close() error { return f.h.close(false) }

// Crash closes the file abruptly; incomplete transactions roll back on reopen.
func (f *SmartFile) Crash() error { return f.h.close(true) }

// Begin starts a transaction.
func (f *SmartFile) Begin() (*Txn, error) { return f.h.begin() }

// Size returns the file size in bytes as seen by tx.
func (f *SmartFile) Size(tx *Txn) (int64, error) {
	p, t, err := f.h.prep(tx)
	if err != nil {
		return 0, err
	}
	n := C.ns_smfile_size(p, t)
	if n < 0 {
		return 0, f.h.err("ns_smfile_size")
	}
	return int64(n), nil
}

// Insert inserts src at byte offset, shifting later bytes right.
func (f *SmartFile) Insert(tx *Txn, src []byte, offset int64) (int64, error) {
	p, t, err := f.h.prep(tx)
	if err != nil {
		return 0, err
	}
	n := C.ns_smfile_insert(p, t, bufPtr(src), C.sb_size(offset), C.b_size(len(src)))
	runtime.KeepAlive(src)
	if n < 0 {
		return 0, f.h.err("ns_smfile_insert")
	}
	return int64(n), nil
}

// Write overwrites len(src)/size elements of size bytes each, starting at
// byte offset and advancing stride elements per write. len(src) must be a
// multiple of size.
func (f *SmartFile) Write(tx *Txn, src []byte, size uint32, offset, stride int64) (int64, error) {
	nelem, err := elemCount(len(src), size)
	if err != nil {
		return 0, err
	}
	p, t, err := f.h.prep(tx)
	if err != nil {
		return 0, err
	}
	n := C.ns_smfile_write(p, t, bufPtr(src), C.t_size(size), C.sb_size(offset), C.sb_size(stride), C.b_size(nelem))
	runtime.KeepAlive(src)
	if n < 0 {
		return 0, f.h.err("ns_smfile_write")
	}
	return int64(n), nil
}

// Read reads len(dest)/size elements of size bytes each into dest, starting
// at byte offset and advancing stride elements per read. len(dest) must be a
// multiple of size.
func (f *SmartFile) Read(tx *Txn, dest []byte, size uint32, offset, stride int64) (int64, error) {
	nelem, err := elemCount(len(dest), size)
	if err != nil {
		return 0, err
	}
	p, t, err := f.h.prep(tx)
	if err != nil {
		return 0, err
	}
	n := C.ns_smfile_read(p, t, bufPtr(dest), C.t_size(size), C.sb_size(offset), C.sb_size(stride), C.b_size(nelem))
	runtime.KeepAlive(dest)
	if n < 0 {
		return 0, f.h.err("ns_smfile_read")
	}
	return int64(n), nil
}

// Remove removes nelem elements of size bytes each, starting at byte offset
// and advancing stride elements per removal. If dest is non-nil the removed
// bytes are copied into it, and it must hold at least nelem*size bytes.
func (f *SmartFile) Remove(tx *Txn, dest []byte, size uint32, offset, stride int64, nelem uint64) (int64, error) {
	if size == 0 {
		return 0, fmt.Errorf("numstore: element size must be > 0")
	}
	if dest != nil {
		if nelem > math.MaxUint64/uint64(size) || uint64(len(dest)) < nelem*uint64(size) {
			return 0, fmt.Errorf("numstore: dest holds %d bytes, need %d elements of %d bytes", len(dest), nelem, size)
		}
	}
	p, t, err := f.h.prep(tx)
	if err != nil {
		return 0, err
	}
	n := C.ns_smfile_remove(p, t, bufPtr(dest), C.t_size(size), C.sb_size(offset), C.sb_size(stride), C.b_size(nelem))
	runtime.KeepAlive(dest)
	if n < 0 {
		return 0, f.h.err("ns_smfile_remove")
	}
	return int64(n), nil
}

func elemCount(nbytes int, size uint32) (uint64, error) {
	if size == 0 {
		return 0, fmt.Errorf("numstore: element size must be > 0")
	}
	if uint64(nbytes)%uint64(size) != 0 {
		return 0, fmt.Errorf("numstore: buffer length %d is not a multiple of element size %d", nbytes, size)
	}
	return uint64(nbytes) / uint64(size), nil
}
