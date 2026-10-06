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

// Package numstore provides Go bindings for the numstore C library.
//
// Two kinds of handle are exposed:
//
//   - *DB, opened with Open, runs numstore queries ("create foo u32",
//     "read foo[0:10]", ...).
//   - *SmartFile, opened with OpenSmartFile, is a transactional byte file
//     supporting insert/read/write/remove at arbitrary offsets.
//
// Both hand out *Txn values with Begin. Every method that takes a *Txn also
// accepts nil, which passes NULL through to the C library.
//
// Queries are passed to C verbatim (through a "%s" format), so a '%' in a
// query is never interpreted as a format directive. Build the query string in
// Go with fmt.Sprintf if you need formatting.
package numstore

/*
#cgo CFLAGS: -I${SRCDIR}
#cgo LDFLAGS: -lnumstore

#include <stdlib.h>
#include "numstore.h"

// cgo cannot call variadic C functions, so each printf-style entry point gets
// a fixed-arity shim that forwards the query through "%s".

static nsdb_plan_t *go_ns_plan_create(nsdb_t *db, const char *q) {
	return ns_plan_fcreate(db, "%s", q);
}

static int go_ns_execute(nsdb_t *db, txn_t *tx, const char *q) {
	return ns_execute(db, tx, "%s", q);
}

static nsdb_var_t *go_ns_get_var(nsdb_t *db, txn_t *tx, const char *q) {
	return ns_get_var(db, tx, "%s", q);
}

static sb_size go_ns_read(nsdb_t *db, txn_t *tx, void *dest, b_size dlen, const char *q) {
	return ns_read(db, tx, dest, dlen, "%s", q);
}

static sb_size go_ns_write(nsdb_t *db, txn_t *tx, const void *src, b_size dlen, const char *q) {
	return ns_write(db, tx, src, dlen, "%s", q);
}

static void *go_ns_malloc(nsdb_t *db, txn_t *tx, b_size *dlen, const char *q) {
	return ns_malloc(db, tx, dlen, "%s", q);
}
*/
import "C"

import (
	"errors"
	"runtime"
	"sync"
	"unsafe"
)

// End is NS_END / SMF_END: an offset meaning "the end of the variable/file".
const End int64 = 1<<63 - 1

var (
	// ErrClosed is returned when a handle is used after Close or Crash.
	ErrClosed = errors.New("numstore: handle is closed")
	// ErrTxnDone is returned when a transaction is used after Commit or Rollback.
	ErrTxnDone = errors.New("numstore: transaction already committed or rolled back")
	// ErrPlanFreed is returned when a plan is used after Free.
	ErrPlanFreed = errors.New("numstore: plan has been freed")
	// ErrWrongHandle is returned when a transaction is used with a handle
	// other than the one that began it.
	ErrWrongHandle = errors.New("numstore: transaction belongs to a different handle")
)

// Error is an error reported by the C library.
type Error struct {
	Op  string // the C function that failed, e.g. "ns_read"
	Msg string // the library's error string, if any
}

func (e *Error) Error() string {
	if e.Msg == "" {
		return "numstore: " + e.Op + " failed"
	}
	return "numstore: " + e.Op + ": " + e.Msg
}

// handle is the shared state behind DB and SmartFile.
type handle struct {
	mu    sync.Mutex
	p     *C.nsdb_t
	plans map[*Plan]struct{}
}

func (h *handle) ptr() (*C.nsdb_t, error) {
	h.mu.Lock()
	defer h.mu.Unlock()
	if h.p == nil {
		return nil, ErrClosed
	}
	return h.p, nil
}

func (h *handle) err(op string) error {
	h.mu.Lock()
	p := h.p
	h.mu.Unlock()
	var msg string
	if p != nil {
		if s := C.ns_strerror(p); s != nil {
			msg = C.GoString(s)
		}
	}
	return &Error{Op: op, Msg: msg}
}

// txp validates tx against h and returns the C pointer (NULL for a nil tx).
func (h *handle) txp(tx *Txn) (*C.txn_t, error) {
	if tx == nil {
		return nil, nil
	}
	if tx.h != h {
		return nil, ErrWrongHandle
	}
	tx.mu.Lock()
	defer tx.mu.Unlock()
	if tx.p == nil {
		return nil, ErrTxnDone
	}
	return tx.p, nil
}

// prep returns both pointers needed for a call, or the first error.
func (h *handle) prep(tx *Txn) (*C.nsdb_t, *C.txn_t, error) {
	p, err := h.ptr()
	if err != nil {
		return nil, nil, err
	}
	t, err := h.txp(tx)
	if err != nil {
		return nil, nil, err
	}
	return p, t, nil
}

func (h *handle) close(crash bool) error {
	h.mu.Lock()
	p := h.p
	if p == nil {
		h.mu.Unlock()
		return ErrClosed
	}
	// Plans must be freed before the database is closed.
	for pl := range h.plans {
		pl.freeLocked()
	}
	h.plans = nil
	h.mu.Unlock()

	var rc C.int
	op := "ns_close"
	if crash {
		op = "ns_crash"
		rc = C.ns_crash(p)
	} else {
		rc = C.ns_close(p) // blocks while a transaction is open
	}
	var err error
	if rc < 0 {
		err = h.err(op)
	}

	h.mu.Lock()
	h.p = nil
	h.mu.Unlock()
	return err
}

func (h *handle) begin() (*Txn, error) {
	p, err := h.ptr()
	if err != nil {
		return nil, err
	}
	t := C.ns_begin(p)
	if t == nil {
		return nil, h.err("ns_begin")
	}
	return &Txn{h: h, p: t}, nil
}

// Cleanup removes a numstore database or smart file at path.
func Cleanup(path string) error {
	cp := C.CString(path)
	defer C.free(unsafe.Pointer(cp))
	if C.ns_cleanup(cp) < 0 {
		return &Error{Op: "ns_cleanup"}
	}
	return nil
}

// ---------------------------------------------------------------- Transactions

// Txn is a transaction on a DB or SmartFile.
type Txn struct {
	h  *handle
	mu sync.Mutex
	p  *C.txn_t
}

func (tx *Txn) finish(commit bool) error {
	p, err := tx.h.ptr()
	if err != nil {
		return err
	}
	tx.mu.Lock()
	t := tx.p
	tx.p = nil
	tx.mu.Unlock()
	if t == nil {
		return ErrTxnDone
	}
	if commit {
		if C.ns_commit(p, t) < 0 {
			return tx.h.err("ns_commit")
		}
	} else if C.ns_rollback(p, t) < 0 {
		return tx.h.err("ns_rollback")
	}
	return nil
}

// Commit commits the transaction. The Txn cannot be used afterwards.
func (tx *Txn) Commit() error { return tx.finish(true) }

// Rollback rolls back the transaction. The Txn cannot be used afterwards.
func (tx *Txn) Rollback() error { return tx.finish(false) }

// ---------------------------------------------------------------- DB

// DB is an open numstore database.
type DB struct{ h *handle }

// Open opens (or creates) the numstore database at path.
func Open(path string) (*DB, error) {
	cp := C.CString(path)
	defer C.free(unsafe.Pointer(cp))
	p := C.ns_open(cp)
	if p == nil {
		return nil, &Error{Op: "ns_open", Msg: path}
	}
	return &DB{h: &handle{p: p, plans: map[*Plan]struct{}{}}}, nil
}

// Close closes the database gracefully, freeing any plans still open.
// If a transaction is still open, Close blocks until it finishes.
func (db *DB) Close() error { return db.h.close(false) }

// Crash closes the database abruptly. Incomplete transactions are rolled
// back on the next Open.
func (db *DB) Crash() error { return db.h.close(true) }

// Begin starts a transaction.
func (db *DB) Begin() (*Txn, error) { return db.h.begin() }

// Execute runs a query that takes no data buffer, e.g. "create foo u32".
func (db *DB) Execute(tx *Txn, query string) error {
	p, t, err := db.h.prep(tx)
	if err != nil {
		return err
	}
	cq := C.CString(query)
	defer C.free(unsafe.Pointer(cq))
	if C.go_ns_execute(p, t, cq) < 0 {
		return db.h.err("ns_execute")
	}
	return nil
}

// GetVar returns the variable a query refers to, e.g. "get foo".
// The returned Var should be released with Free.
func (db *DB) GetVar(tx *Txn, query string) (*Var, error) {
	p, t, err := db.h.prep(tx)
	if err != nil {
		return nil, err
	}
	cq := C.CString(query)
	defer C.free(unsafe.Pointer(cq))
	v := C.go_ns_get_var(p, t, cq)
	if v == nil {
		return nil, db.h.err("ns_get_var")
	}
	return newVar(v), nil
}

// Read runs a READ or REMOVE query, filling dest. It returns the number of
// bytes the library reported.
func (db *DB) Read(tx *Txn, dest []byte, query string) (int64, error) {
	p, t, err := db.h.prep(tx)
	if err != nil {
		return 0, err
	}
	cq := C.CString(query)
	defer C.free(unsafe.Pointer(cq))
	n := C.go_ns_read(p, t, bufPtr(dest), C.b_size(len(dest)), cq)
	runtime.KeepAlive(dest)
	if n < 0 {
		return 0, db.h.err("ns_read")
	}
	return int64(n), nil
}

// Write runs an INSERT or WRITE query, sourcing data from src. It returns
// the number of bytes the library reported.
func (db *DB) Write(tx *Txn, src []byte, query string) (int64, error) {
	p, t, err := db.h.prep(tx)
	if err != nil {
		return 0, err
	}
	cq := C.CString(query)
	defer C.free(unsafe.Pointer(cq))
	n := C.go_ns_write(p, t, bufPtr(src), C.b_size(len(src)), cq)
	runtime.KeepAlive(src)
	if n < 0 {
		return 0, db.h.err("ns_write")
	}
	return int64(n), nil
}

// ReadAll runs a READ or REMOVE query and returns all resulting bytes,
// letting the library size the buffer (ns_malloc).
func (db *DB) ReadAll(tx *Txn, query string) ([]byte, error) {
	p, t, err := db.h.prep(tx)
	if err != nil {
		return nil, err
	}
	cq := C.CString(query)
	defer C.free(unsafe.Pointer(cq))
	var n C.b_size
	buf := C.go_ns_malloc(p, t, &n, cq)
	if buf == nil {
		return nil, db.h.err("ns_malloc")
	}
	return takeCBuf(buf, n), nil
}

// Prepare creates a reusable query plan. Plans should be freed with Free;
// any still open are freed automatically by Close/Crash.
func (db *DB) Prepare(query string) (*Plan, error) {
	p, err := db.h.ptr()
	if err != nil {
		return nil, err
	}
	cq := C.CString(query)
	defer C.free(unsafe.Pointer(cq))
	cp := C.go_ns_plan_create(p, cq)
	if cp == nil {
		return nil, db.h.err("ns_plan_fcreate")
	}
	pl := &Plan{h: db.h, p: cp}
	db.h.mu.Lock()
	defer db.h.mu.Unlock()
	if db.h.p == nil { // closed concurrently
		C.ns_plan_free(cp)
		return nil, ErrClosed
	}
	db.h.plans[pl] = struct{}{}
	return pl, nil
}

// ---------------------------------------------------------------- Plans

// Plan is a prepared query. It is bound to the DB that created it.
type Plan struct {
	h *handle
	p *C.nsdb_plan_t // guarded by h.mu
}

// freeLocked frees the plan; h.mu must be held.
func (pl *Plan) freeLocked() {
	if pl.p != nil {
		C.ns_plan_free(pl.p)
		pl.p = nil
	}
	delete(pl.h.plans, pl)
}

// Free releases the plan. It is safe to call more than once.
func (pl *Plan) Free() {
	pl.h.mu.Lock()
	defer pl.h.mu.Unlock()
	pl.freeLocked()
}

func (pl *Plan) prep(tx *Txn) (*C.nsdb_plan_t, *C.txn_t, error) {
	if _, err := pl.h.ptr(); err != nil {
		return nil, nil, err
	}
	pl.h.mu.Lock()
	p := pl.p
	pl.h.mu.Unlock()
	if p == nil {
		return nil, nil, ErrPlanFreed
	}
	t, err := pl.h.txp(tx)
	if err != nil {
		return nil, nil, err
	}
	return p, t, nil
}

func (pl *Plan) err(op string) error {
	pl.h.mu.Lock()
	p := pl.p
	pl.h.mu.Unlock()
	var msg string
	if p != nil {
		if s := C.ns_plan_strerror(p); s != nil {
			msg = C.GoString(s)
		}
	}
	return &Error{Op: op, Msg: msg}
}

// Execute runs a plan that takes no data buffer.
func (pl *Plan) Execute(tx *Txn) error {
	p, t, err := pl.prep(tx)
	if err != nil {
		return err
	}
	if C.ns_plan_execute(p, t) < 0 {
		return pl.err("ns_plan_execute")
	}
	return nil
}

// GetVar returns the variable the plan refers to.
func (pl *Plan) GetVar(tx *Txn) (*Var, error) {
	p, t, err := pl.prep(tx)
	if err != nil {
		return nil, err
	}
	v := C.ns_plan_get_var(p, t)
	if v == nil {
		return nil, pl.err("ns_plan_get_var")
	}
	return newVar(v), nil
}

// Read runs a READ/REMOVE plan into dest.
func (pl *Plan) Read(tx *Txn, dest []byte) (int64, error) {
	p, t, err := pl.prep(tx)
	if err != nil {
		return 0, err
	}
	n := C.ns_plan_read(p, t, bufPtr(dest), C.b_size(len(dest)))
	runtime.KeepAlive(dest)
	if n < 0 {
		return 0, pl.err("ns_plan_read")
	}
	return int64(n), nil
}

// Write runs an INSERT/WRITE plan sourcing data from src.
func (pl *Plan) Write(tx *Txn, src []byte) (int64, error) {
	p, t, err := pl.prep(tx)
	if err != nil {
		return 0, err
	}
	n := C.ns_plan_write(p, t, bufPtr(src), C.b_size(len(src)))
	runtime.KeepAlive(src)
	if n < 0 {
		return 0, pl.err("ns_plan_write")
	}
	return int64(n), nil
}

// ReadAll runs a READ/REMOVE plan and returns all resulting bytes.
func (pl *Plan) ReadAll(tx *Txn) ([]byte, error) {
	p, t, err := pl.prep(tx)
	if err != nil {
		return nil, err
	}
	var n C.b_size
	buf := C.ns_plan_malloc(p, t, &n)
	if buf == nil {
		return nil, pl.err("ns_plan_malloc")
	}
	return takeCBuf(buf, n), nil
}

// ---------------------------------------------------------------- Variables

// Var is a numstore variable. Vars own their memory independently of the
// DB, so they may be freed before or after the DB is closed. A finalizer
// frees a Var that is never explicitly freed.
type Var struct {
	mu sync.Mutex
	p  *C.nsdb_var_t
}

func newVar(p *C.nsdb_var_t) *Var {
	v := &Var{p: p}
	runtime.SetFinalizer(v, (*Var).Free)
	return v
}

// Len returns the variable's length in elements (not bytes).
// It returns 0 after Free.
func (v *Var) Len() uint64 {
	v.mu.Lock()
	defer v.mu.Unlock()
	if v.p == nil {
		return 0
	}
	return uint64(C.ns_var_len(v.p))
}

// Free releases the variable. It is safe to call more than once.
func (v *Var) Free() {
	v.mu.Lock()
	defer v.mu.Unlock()
	if v.p != nil {
		C.ns_var_free(v.p)
		v.p = nil
		runtime.SetFinalizer(v, nil)
	}
}

// ---------------------------------------------------------------- helpers

func bufPtr(b []byte) unsafe.Pointer {
	if len(b) == 0 {
		return nil
	}
	return unsafe.Pointer(&b[0])
}

// takeCBuf copies a malloc'd C buffer into Go memory and frees it.
func takeCBuf(buf unsafe.Pointer, n C.b_size) []byte {
	defer C.free(buf)
	if n == 0 {
		return []byte{}
	}
	out := make([]byte, uint64(n))
	copy(out, unsafe.Slice((*byte)(buf), uint64(n)))
	return out
}
