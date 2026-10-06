// numstore.cpp - non-template parts of the numstore C++ wrapper.

#include "numstore.hpp"

#include <utility>

namespace numstore {

//////////////////////////////////// detail

std::string detail::message(const char *what, const char *detail) {
    std::string m(what);
    if (detail && *detail) {
        m += ": ";
        m += detail;
    }
    return m;
}

//////////////////////////////////// Var

Var::Var(nsdb_var_t *v) : var_(v) {}

b_size Var::length() const { return ns_var_len(var_.get()); }

nsdb_var_t *Var::raw() const { return var_.get(); }

void Var::Deleter::operator()(nsdb_var_t *v) const { ns_var_free(v); }

//////////////////////////////////// Handle

Handle::Handle(nsdb_t *ns) : ns_(ns) {}

Handle::Handle(Handle &&o) noexcept : ns_(std::exchange(o.ns_, nullptr)) {}

Handle &Handle::operator=(Handle &&o) noexcept {
    if (this != &o) {
        reset();
        ns_ = std::exchange(o.ns_, nullptr);
    }
    return *this;
}

Handle::~Handle() { reset(); }

void Handle::reset() noexcept {
    if (ns_) ns_close(std::exchange(ns_, nullptr));
}

void Handle::close() {
    if (!ns_) return;
    nsdb_t *n = std::exchange(ns_, nullptr);
    if (ns_close(n) < 0) throw Error("ns_close failed");
}

void Handle::crash() {
    if (!ns_) return;
    nsdb_t *n = std::exchange(ns_, nullptr);
    if (ns_crash(n) < 0) throw Error("ns_crash failed");
}

void Handle::remove(const std::string &path) {
    if (ns_cleanup(path.c_str()) < 0)
        throw Error("ns_cleanup failed for " + path);
}

Transaction Handle::begin() { return Transaction(checked()); }

std::string Handle::last_error() const {
    const char *e = ns_ ? ns_strerror(ns_) : nullptr;
    return e ? e : "";
}

bool Handle::is_open() const { return ns_ != nullptr; }

nsdb_t *Handle::raw() const { return ns_; }

nsdb_t *Handle::checked() const {
    if (!ns_) throw Error("numstore handle is closed");
    return ns_;
}

void Handle::fail(const char *what) const {
    throw Error(detail::message(what, ns_ ? ns_strerror(ns_) : nullptr));
}

//////////////////////////////////// Transaction

Transaction::Transaction(nsdb_t *ns) : ns_(ns), tx_(ns_begin(ns)) {
    if (!tx_)
        throw Error(detail::message("ns_begin failed", ns_strerror(ns_)));
}

Transaction::Transaction(Transaction &&o) noexcept
    : ns_(o.ns_), tx_(std::exchange(o.tx_, nullptr)) {}

Transaction &Transaction::operator=(Transaction &&o) noexcept {
    if (this != &o) {
        abandon();
        ns_ = o.ns_;
        tx_ = std::exchange(o.tx_, nullptr);
    }
    return *this;
}

Transaction::~Transaction() { abandon(); }

void Transaction::commit() {
    txn_t *t = require_active();
    tx_      = nullptr;
    if (ns_commit(ns_, t) < 0)
        throw Error(detail::message("ns_commit failed", ns_strerror(ns_)));
}

void Transaction::rollback() {
    txn_t *t = require_active();
    tx_      = nullptr;
    if (ns_rollback(ns_, t) < 0)
        throw Error(detail::message("ns_rollback failed", ns_strerror(ns_)));
}

txn_t *Transaction::release() noexcept { return std::exchange(tx_, nullptr); }

bool Transaction::active() const { return tx_ != nullptr; }

txn_t *Transaction::raw() const { return tx_; }

txn_t *Transaction::require_active() const {
    if (!tx_) throw Error("transaction already finished");
    return tx_;
}

void Transaction::abandon() noexcept {
    if (tx_) ns_rollback(ns_, std::exchange(tx_, nullptr));
}

//////////////////////////////////// Plan

Plan::Plan(nsdb_plan_t *p) : plan_(p) {}

void Plan::Deleter::operator()(nsdb_plan_t *p) const { ns_plan_free(p); }

void Plan::execute(Transaction &tx) {
    check(ns_plan_execute(plan_.get(), tx.raw()), "ns_plan_execute failed");
}

Var Plan::get_var(Transaction &tx) {
    nsdb_var_t *v = ns_plan_get_var(plan_.get(), tx.raw());
    if (!v) fail("ns_plan_get_var failed");
    return Var(v);
}

sb_size Plan::read(Transaction &tx, void *dest, b_size bytes) {
    return check(ns_plan_read(plan_.get(), tx.raw(), dest, bytes),
                 "ns_plan_read failed");
}

sb_size Plan::write(Transaction &tx, const void *src, b_size bytes) {
    return check(ns_plan_write(plan_.get(), tx.raw(), src, bytes),
                 "ns_plan_write failed");
}

void *Plan::malloc_raw(Transaction &tx, b_size *len) {
    void *p = ns_plan_malloc(plan_.get(), tx.raw(), len);
    if (!p) fail("ns_plan_malloc failed");
    return p;
}

std::string Plan::last_error() const {
    const char *e = ns_plan_strerror(plan_.get());
    return e ? e : "";
}

nsdb_plan_t *Plan::raw() const { return plan_.get(); }

void Plan::fail(const char *what) const {
    throw Error(detail::message(what, ns_plan_strerror(plan_.get())));
}

//////////////////////////////////// Database

Database::Database(const std::string &path) : Handle(ns_open(path.c_str())) {
    if (!is_open()) throw Error("ns_open failed for " + path);
}

void Database::execute(Transaction &tx, const std::string &query) {
    check(ns_execute(checked(), tx.raw(), "%s", query.c_str()),
          "ns_execute failed");
}

Var Database::get_var(Transaction &tx, const std::string &query) {
    nsdb_var_t *v = ns_get_var(checked(), tx.raw(), "%s", query.c_str());
    if (!v) fail("ns_get_var failed");
    return Var(v);
}

Plan Database::plan(const std::string &query) {
    nsdb_plan_t *p = ns_plan_fcreate(checked(), "%s", query.c_str());
    if (!p) fail("ns_plan_fcreate failed");
    return Plan(p);
}

sb_size Database::read(Transaction &tx, void *dest, b_size bytes,
                       const std::string &query) {
    return check(ns_read(checked(), tx.raw(), dest, bytes, "%s", query.c_str()),
                 "ns_read failed");
}

sb_size Database::write(Transaction &tx, const void *src, b_size bytes,
                        const std::string &query) {
    return check(ns_write(checked(), tx.raw(), src, bytes, "%s", query.c_str()),
                 "ns_write failed");
}

void *Database::malloc_raw(Transaction &tx, b_size *len, const std::string &query) {
    void *p = ns_malloc(checked(), tx.raw(), len, "%s", query.c_str());
    if (!p) fail("ns_malloc failed");
    return p;
}

//////////////////////////////////// SmartFile

SmartFile::SmartFile(const std::string &path) : Handle(ns_smfile_open(path.c_str())) {
    if (!is_open()) throw Error("ns_smfile_open failed for " + path);
}

sb_size SmartFile::size(Transaction &tx) {
    return check(ns_smfile_size(checked(), tx.raw()), "ns_smfile_size failed");
}

sb_size SmartFile::insert(Transaction &tx, const void *src, b_size bytes,
                          sb_size byte_offset) {
    return check(ns_smfile_insert(checked(), tx.raw(), src, byte_offset, bytes),
                 "ns_smfile_insert failed");
}

sb_size SmartFile::write_raw(Transaction &tx, const void *src, t_size elem_size,
                             sb_size byte_offset, sb_size stride, b_size count) {
    return check(ns_smfile_write(checked(), tx.raw(), src, elem_size,
                                 byte_offset, stride, count),
                 "ns_smfile_write failed");
}

sb_size SmartFile::read_raw(Transaction &tx, void *dest, t_size elem_size,
                            sb_size byte_offset, sb_size stride, b_size count) {
    return check(ns_smfile_read(checked(), tx.raw(), dest, elem_size,
                                byte_offset, stride, count),
                 "ns_smfile_read failed");
}

sb_size SmartFile::remove_raw(Transaction &tx, void *dest, t_size elem_size,
                              sb_size byte_offset, sb_size stride, b_size count) {
    return check(ns_smfile_remove(checked(), tx.raw(), dest, elem_size,
                                  byte_offset, stride, count),
                 "ns_smfile_remove failed");
}

} // namespace numstore
