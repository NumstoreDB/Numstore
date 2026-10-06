// bindings.cpp - Rcpp glue between R and the numstore C++ wrapper.
//
// Lifetime model
// --------------
// R frees objects through GC finalizers, in no guaranteed order. The C API
// needs plans freed and transactions finished *before* the database closes.
// So every database box keeps a registry of its live transactions and plans:
//
//   - closing / finalizing a database first rolls back its open
//     transactions and frees its plans, then closes;
//   - a transaction or plan finalized later sees owner == nullptr and does
//     nothing.
//
// Transactions and plans also "protect" their database's R object, so the
// database is never collected while one of them is still reachable.

#include <Rcpp.h>

#include "numstore.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

namespace {

using numstore::Database;
using numstore::Handle;
using numstore::Plan;
using numstore::SmartFile;
using numstore::Transaction;

//////////////////////////////////// Boxes

struct TxnBox;
struct PlanBox;

struct DbBox {
    std::unique_ptr<Database>  db;  // exactly one of db / smf is set
    std::unique_ptr<SmartFile> smf;
    std::set<TxnBox *>         txns;
    std::set<PlanBox *>        plans;

    Handle &handle() {
        if (db) return *db;
        return *smf;
    }

    // rollback = false is used before a crash: transactions are dropped
    // without being touched, since ns_crash invalidates them anyway.
    void release_children(bool rollback = true) noexcept;

    ~DbBox() { release_children(); }
};

struct TxnBox {
    DbBox                     *owner;
    std::optional<Transaction> tx;

    TxnBox(DbBox *o, Transaction t) : owner(o), tx(std::move(t)) {
        owner->txns.insert(this);
    }
    ~TxnBox() {
        tx.reset();  // rolls back if still active
        if (owner) owner->txns.erase(this);
    }
};

struct PlanBox {
    DbBox              *owner;
    std::optional<Plan> plan;

    PlanBox(DbBox *o, Plan p) : owner(o), plan(std::move(p)) {
        owner->plans.insert(this);
    }
    ~PlanBox() {
        plan.reset();
        if (owner) owner->plans.erase(this);
    }
};

void DbBox::release_children(bool rollback) noexcept {
    for (PlanBox *p : plans) {
        p->plan.reset();
        p->owner = nullptr;
    }
    plans.clear();
    for (TxnBox *t : txns) {
        if (!rollback && t->tx) t->tx->release();
        t->tx.reset();
        t->owner = nullptr;
    }
    txns.clear();
}

// Finalize on R exit too, so databases close gracefully when R quits.
template <typename T>
using Ptr = Rcpp::XPtr<T, Rcpp::PreserveStorage, Rcpp::standard_delete_finalizer<T>, true>;

DbBox &get_handle(SEXP h) {
    Ptr<DbBox> p(h);
    if (!p.get())
        Rcpp::stop("numstore handle is invalid (handles cannot be saved and reloaded)");
    return *p;
}

TxnBox &get_txn(SEXP x) {
    Ptr<TxnBox> p(x);
    if (!p.get()) Rcpp::stop("transaction is invalid (it cannot be saved and reloaded)");
    if (!p->owner) Rcpp::stop("the transaction's database has been closed");
    if (!p->tx || !p->tx->active())
        Rcpp::stop("transaction has already been committed or rolled back");
    return *p;
}

PlanBox &get_plan(SEXP x) {
    Ptr<PlanBox> p(x);
    if (!p.get()) Rcpp::stop("plan is invalid (it cannot be saved and reloaded)");
    if (!p->owner) Rcpp::stop("the plan's database has been closed");
    if (!p->plan) Rcpp::stop("plan has been freed");
    return *p;
}

Database &txn_db(TxnBox &t) {
    if (!t.owner->db) Rcpp::stop("this operation needs a database transaction, not a smart file one");
    return *t.owner->db;
}

SmartFile &txn_smf(TxnBox &t) {
    if (!t.owner->smf) Rcpp::stop("this operation needs a smart file transaction, not a database one");
    return *t.owner->smf;
}

//////////////////////////////////// Element types <-> R vectors

enum class Ty { I8, U8, I16, U16, I32, U32, I64, U64, F32, F64 };

Ty parse_type(const std::string &s) {
    if (s == "i8") return Ty::I8;
    if (s == "u8") return Ty::U8;
    if (s == "i16") return Ty::I16;
    if (s == "u16") return Ty::U16;
    if (s == "i32") return Ty::I32;
    if (s == "u32") return Ty::U32;
    if (s == "i64") return Ty::I64;
    if (s == "u64") return Ty::U64;
    if (s == "f32") return Ty::F32;
    if (s == "f64") return Ty::F64;
    Rcpp::stop("unknown element type '%s'", s);
}

// Types that always fit in R's 32-bit integer come back as integer vectors;
// everything else (u32, 64-bit ints, floats) comes back as double.
template <typename T>
constexpr bool fits_r_int = std::is_integral_v<T> && (sizeof(T) < 4 || std::is_same_v<T, int32_t>);

template <typename T>
SEXP decode_as(const unsigned char *p, size_t nbytes) {
    const size_t count = nbytes / sizeof(T);
    if constexpr (fits_r_int<T>) {
        Rcpp::IntegerVector out(count);
        for (size_t i = 0; i < count; ++i) {
            T v;
            std::memcpy(&v, p + i * sizeof(T), sizeof(T));
            out[i] = static_cast<int>(v);
        }
        return out;
    } else {
        Rcpp::NumericVector out(count);
        for (size_t i = 0; i < count; ++i) {
            T v;
            std::memcpy(&v, p + i * sizeof(T), sizeof(T));
            out[i] = static_cast<double>(v);
        }
        return out;
    }
}

template <typename T>
std::vector<unsigned char> encode_as(SEXP x) {
    Rcpp::NumericVector       v(x);  // coerces integer / logical
    std::vector<unsigned char> out(v.size() * sizeof(T));
    for (R_xlen_t i = 0; i < v.size(); ++i) {
        const double d = v[i];
        if constexpr (std::is_integral_v<T>) {
            if (std::isnan(d))
                Rcpp::stop("element %d is NA/NaN, which an integer type cannot store", i + 1);
            const double lo = static_cast<double>(std::numeric_limits<T>::min());
            const double hi = static_cast<double>(std::numeric_limits<T>::max()) + 1.0;
            if (d != std::trunc(d) || d < lo || d >= hi)
                Rcpp::stop("element %d (%g) does not fit the requested integer type", i + 1, d);
        }
        const T t = static_cast<T>(d);
        std::memcpy(out.data() + i * sizeof(T), &t, sizeof(T));
    }
    return out;
}

#define NS_DISPATCH(ty, FN, ...)                       \
    switch (ty) {                                      \
    case Ty::I8: return FN<int8_t>(__VA_ARGS__);       \
    case Ty::U8: return FN<uint8_t>(__VA_ARGS__);      \
    case Ty::I16: return FN<int16_t>(__VA_ARGS__);     \
    case Ty::U16: return FN<uint16_t>(__VA_ARGS__);    \
    case Ty::I32: return FN<int32_t>(__VA_ARGS__);     \
    case Ty::U32: return FN<uint32_t>(__VA_ARGS__);    \
    case Ty::I64: return FN<int64_t>(__VA_ARGS__);     \
    case Ty::U64: return FN<uint64_t>(__VA_ARGS__);    \
    case Ty::F32: return FN<float>(__VA_ARGS__);       \
    case Ty::F64: return FN<double>(__VA_ARGS__);      \
    }                                                  \
    Rcpp::stop("unreachable");

SEXP decode(Ty ty, const unsigned char *p, size_t n) { NS_DISPATCH(ty, decode_as, p, n) }
std::vector<unsigned char> encode(Ty ty, SEXP x) { NS_DISPATCH(ty, encode_as, x) }

template <typename T>
size_t size_as() { return sizeof(T); }
size_t type_size(Ty ty) { NS_DISPATCH(ty, size_as) }

#undef NS_DISPATCH

sb_size to_sb(double d, const char *what) {
    if (std::isnan(d) || d != std::trunc(d)) Rcpp::stop("%s must be a whole number", what);
    return static_cast<sb_size>(d);
}

size_t to_count(double d) {
    if (std::isnan(d) || d < 0 || d != std::trunc(d))
        Rcpp::stop("n must be a non-negative whole number");
    return static_cast<size_t>(d);
}

} // namespace

//////////////////////////////////// Handles

// [[Rcpp::export]]
SEXP cpp_open(std::string path) {
    auto box = std::make_unique<DbBox>();
    box->db  = std::make_unique<Database>(path);
    return Ptr<DbBox>(box.release(), true);
}

// [[Rcpp::export]]
SEXP cpp_smfile_open(std::string path) {
    auto box = std::make_unique<DbBox>();
    box->smf = std::make_unique<SmartFile>(path);
    return Ptr<DbBox>(box.release(), true);
}

// [[Rcpp::export]]
void cpp_close(SEXP h) {
    DbBox &b = get_handle(h);
    b.release_children(true);
    b.handle().close();
}

// [[Rcpp::export]]
void cpp_crash(SEXP h) {
    DbBox &b = get_handle(h);
    b.release_children(false);
    b.handle().crash();
}

// [[Rcpp::export]]
bool cpp_is_open(SEXP h) {
    Ptr<DbBox> p(h);
    return p.get() && p->handle().is_open();
}

// [[Rcpp::export]]
void cpp_cleanup(std::string path) { Handle::remove(path); }

// [[Rcpp::export]]
std::string cpp_last_error(SEXP h) { return get_handle(h).handle().last_error(); }

//////////////////////////////////// Transactions

// [[Rcpp::export]]
SEXP cpp_begin(SEXP h) {
    DbBox &b   = get_handle(h);
    auto   box = std::make_unique<TxnBox>(&b, b.handle().begin());
    return Ptr<TxnBox>(box.release(), true, R_NilValue, h);  // keeps h alive
}

// [[Rcpp::export]]
void cpp_commit(SEXP x) { get_txn(x).tx->commit(); }

// [[Rcpp::export]]
void cpp_rollback(SEXP x) { get_txn(x).tx->rollback(); }

// [[Rcpp::export]]
bool cpp_txn_active(SEXP x) {
    Ptr<TxnBox> p(x);
    return p.get() && p->owner && p->tx && p->tx->active();
}

//////////////////////////////////// Database queries

// [[Rcpp::export]]
void cpp_execute(SEXP x, std::string query) {
    TxnBox &t = get_txn(x);
    txn_db(t).execute(*t.tx, query);
}

// [[Rcpp::export]]
double cpp_var_len(SEXP x, std::string query) {
    TxnBox &t = get_txn(x);
    return static_cast<double>(txn_db(t).get_var(*t.tx, query).length());
}

// [[Rcpp::export]]
SEXP cpp_read(SEXP x, std::string query, std::string type) {
    const Ty ty    = parse_type(type);
    TxnBox  &t     = get_txn(x);
    auto     bytes = txn_db(t).read_all<unsigned char>(*t.tx, query);
    return decode(ty, bytes.data(), bytes.size());
}

// [[Rcpp::export]]
double cpp_write(SEXP x, std::string query, SEXP values, std::string type) {
    const Ty ty    = parse_type(type);
    auto     bytes = encode(ty, values);
    TxnBox  &t     = get_txn(x);
    return static_cast<double>(
        txn_db(t).write(*t.tx, static_cast<const void *>(bytes.data()), bytes.size(), query));
}

//////////////////////////////////// Plans

// [[Rcpp::export]]
SEXP cpp_plan(SEXP h, std::string query) {
    DbBox &b = get_handle(h);
    if (!b.db) Rcpp::stop("plans need a database, not a smart file");
    auto box = std::make_unique<PlanBox>(&b, b.db->plan(query));
    return Ptr<PlanBox>(box.release(), true, R_NilValue, h);  // keeps h alive
}

namespace {
PlanBox &plan_for(SEXP p, TxnBox &t) {
    PlanBox &pb = get_plan(p);
    if (pb.owner != t.owner) Rcpp::stop("plan and transaction belong to different databases");
    return pb;
}
} // namespace

// [[Rcpp::export]]
void cpp_plan_execute(SEXP p, SEXP x) {
    TxnBox &t = get_txn(x);
    plan_for(p, t).plan->execute(*t.tx);
}

// [[Rcpp::export]]
double cpp_plan_var_len(SEXP p, SEXP x) {
    TxnBox &t = get_txn(x);
    return static_cast<double>(plan_for(p, t).plan->get_var(*t.tx).length());
}

// [[Rcpp::export]]
SEXP cpp_plan_read(SEXP p, SEXP x, std::string type) {
    const Ty ty    = parse_type(type);
    TxnBox  &t     = get_txn(x);
    auto     bytes = plan_for(p, t).plan->read_all<unsigned char>(*t.tx);
    return decode(ty, bytes.data(), bytes.size());
}

// [[Rcpp::export]]
double cpp_plan_write(SEXP p, SEXP x, SEXP values, std::string type) {
    const Ty ty    = parse_type(type);
    auto     bytes = encode(ty, values);
    TxnBox  &t     = get_txn(x);
    return static_cast<double>(
        plan_for(p, t).plan->write(*t.tx, static_cast<const void *>(bytes.data()), bytes.size()));
}

// [[Rcpp::export]]
void cpp_plan_free(SEXP p) {
    Ptr<PlanBox> pb(p);
    if (pb.get()) pb->plan.reset();
}

//////////////////////////////////// Smart files

// [[Rcpp::export]]
double cpp_smf_size(SEXP x) {
    TxnBox &t = get_txn(x);
    return static_cast<double>(txn_smf(t).size(*t.tx));
}

// [[Rcpp::export]]
double cpp_smf_insert(SEXP x, SEXP values, std::string type, double offset) {
    const Ty ty    = parse_type(type);
    auto     bytes = encode(ty, values);
    TxnBox  &t     = get_txn(x);
    return static_cast<double>(
        txn_smf(t).insert(*t.tx, bytes.data(), bytes.size(), to_sb(offset, "offset")));
}

// [[Rcpp::export]]
double cpp_smf_write(SEXP x, SEXP values, std::string type, double offset, double stride) {
    const Ty     ty    = parse_type(type);
    const size_t sz    = type_size(ty);
    auto         bytes = encode(ty, values);
    TxnBox      &t     = get_txn(x);
    return static_cast<double>(txn_smf(t).write_raw(
        *t.tx, bytes.data(), static_cast<t_size>(sz), to_sb(offset, "offset"),
        to_sb(stride, "stride"), bytes.size() / sz));
}

// [[Rcpp::export]]
SEXP cpp_smf_read(SEXP x, double n, std::string type, double offset, double stride) {
    const Ty     ty    = parse_type(type);
    const size_t sz    = type_size(ty);
    const size_t count = to_count(n);
    std::vector<unsigned char> buf(count * sz);
    TxnBox &t = get_txn(x);
    txn_smf(t).read_raw(*t.tx, buf.data(), static_cast<t_size>(sz),
                        to_sb(offset, "offset"), to_sb(stride, "stride"), count);
    return decode(ty, buf.data(), buf.size());
}

// [[Rcpp::export]]
SEXP cpp_smf_remove(SEXP x, double n, std::string type, double offset, double stride, bool keep) {
    const Ty     ty    = parse_type(type);
    const size_t sz    = type_size(ty);
    const size_t count = to_count(n);
    std::vector<unsigned char> buf(keep ? count * sz : 0);
    TxnBox &t = get_txn(x);
    txn_smf(t).remove_raw(*t.tx, keep ? buf.data() : nullptr, static_cast<t_size>(sz),
                          to_sb(offset, "offset"), to_sb(stride, "stride"), count);
    if (!keep) return R_NilValue;
    return decode(ty, buf.data(), buf.size());
}
