// numstore.hpp - C++17 wrapper around the numstore C API.
//
// Design:
//   - RAII handles: Database / SmartFile / Transaction / Plan / Var all
//     release their C resource in the destructor and are move-only.
//   - Errors become numstore::Error exceptions carrying ns_strerror text.
//   - Queries are passed to the C API through "%s", so a query string
//     containing '%' can never be misread as a printf format.
//   - Typed template helpers (read<T>, write<T>, ...) work in elements
//     instead of raw bytes. Templates live here; everything else is in
//     numstore.cpp.
//
// Assumption: functions returning int / sb_size signal failure with a
// negative value, and pointer-returning functions signal failure with NULL.

#ifndef NUMSTORE_HPP
#define NUMSTORE_HPP

extern "C" {
#include "numstore.h"
}

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace numstore {

//////////////////////////////////// Errors

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

namespace detail {

std::string message(const char *what, const char *detail);

template <typename T>
constexpr void require_pod() {
    static_assert(std::is_trivially_copyable_v<T>,
                  "numstore element types must be trivially copyable");
}

// Copy a malloc'd buffer into a vector<T> and free it.
template <typename T>
std::vector<T> take_buffer(void *p, b_size len) {
    std::unique_ptr<void, decltype(&std::free)> guard(p, &std::free);
    std::vector<T> out(len / sizeof(T));
    if (!out.empty()) std::memcpy(out.data(), p, out.size() * sizeof(T));
    return out;
}

} // namespace detail

class Transaction;

//////////////////////////////////// Var

// A variable handle. Owns its own memory, so it may outlive the database.
class Var {
public:
    explicit Var(nsdb_var_t *v);

    b_size length() const;
    nsdb_var_t *raw() const;

private:
    struct Deleter {
        void operator()(nsdb_var_t *v) const;
    };
    std::unique_ptr<nsdb_var_t, Deleter> var_;
};

//////////////////////////////////// Handle (common base for Database / SmartFile)

class Handle {
public:
    Handle(const Handle &)            = delete;
    Handle &operator=(const Handle &) = delete;
    Handle(Handle &&o) noexcept;
    Handle &operator=(Handle &&o) noexcept;
    ~Handle();

    // Graceful close (blocks while a transaction is open). Throws on failure.
    void close();

    // Harsh close; incomplete transactions roll back on next open.
    void crash();

    // Delete the database / smart file at path.
    static void remove(const std::string &path);

    Transaction begin();

    std::string last_error() const;
    bool is_open() const;
    nsdb_t *raw() const;

protected:
    explicit Handle(nsdb_t *ns);

    nsdb_t *checked() const;
    [[noreturn]] void fail(const char *what) const;

    template <typename R>
    R check(R rc, const char *what) const {
        if (rc < 0) fail(what);
        return rc;
    }

private:
    void reset() noexcept;

    nsdb_t *ns_ = nullptr;
};

//////////////////////////////////// Transaction

// Rolls back automatically if destroyed without commit().
class Transaction {
public:
    Transaction(const Transaction &)            = delete;
    Transaction &operator=(const Transaction &) = delete;
    Transaction(Transaction &&o) noexcept;
    Transaction &operator=(Transaction &&o) noexcept;
    ~Transaction();

    void commit();
    void rollback();

    // Detach without committing or rolling back (e.g. after ns_crash,
    // which invalidates every open transaction).
    txn_t *release() noexcept;

    bool active() const;
    txn_t *raw() const;

private:
    friend class Handle;
    explicit Transaction(nsdb_t *ns);

    txn_t *require_active() const;
    void abandon() noexcept;

    nsdb_t *ns_ = nullptr;
    txn_t  *tx_ = nullptr;
};

//////////////////////////////////// Plan

// A prepared query. Must be destroyed before its Database is closed.
class Plan {
public:
    void execute(Transaction &tx);
    Var get_var(Transaction &tx);

    // Raw byte API
    sb_size read(Transaction &tx, void *dest, b_size bytes);
    sb_size write(Transaction &tx, const void *src, b_size bytes);

    // Typed API. read returns number of elements read.
    template <typename T>
    size_t read(Transaction &tx, T *dest, size_t count) {
        detail::require_pod<T>();
        auto n = read(tx, static_cast<void *>(dest), count * sizeof(T));
        return static_cast<size_t>(n) / sizeof(T);
    }

    template <typename T>
    size_t read(Transaction &tx, std::vector<T> &dest) {
        return read(tx, dest.data(), dest.size());
    }

    template <typename T>
    sb_size write(Transaction &tx, const std::vector<T> &src) {
        detail::require_pod<T>();
        return write(tx, static_cast<const void *>(src.data()), src.size() * sizeof(T));
    }

    // Read everything the plan produces into a vector (uses ns_plan_malloc).
    template <typename T>
    std::vector<T> read_all(Transaction &tx) {
        detail::require_pod<T>();
        b_size len = 0;
        void  *p   = malloc_raw(tx, &len);
        return detail::take_buffer<T>(p, len);
    }

    std::string last_error() const;
    nsdb_plan_t *raw() const;

private:
    friend class Database;
    explicit Plan(nsdb_plan_t *p);

    void *malloc_raw(Transaction &tx, b_size *len);
    [[noreturn]] void fail(const char *what) const;

    template <typename R>
    R check(R rc, const char *what) const {
        if (rc < 0) fail(what);
        return rc;
    }

    struct Deleter {
        void operator()(nsdb_plan_t *p) const;
    };
    std::unique_ptr<nsdb_plan_t, Deleter> plan_;
};

//////////////////////////////////// Database

class Database : public Handle {
public:
    explicit Database(const std::string &path);

    // For queries that take no data, e.g. "create foo u32", "delete foo".
    void execute(Transaction &tx, const std::string &query);

    // e.g. "get foo"
    Var get_var(Transaction &tx, const std::string &query);

    Plan plan(const std::string &query);

    // Raw byte API. READ / REMOVE queries; returns bytes read.
    sb_size read(Transaction &tx, void *dest, b_size bytes, const std::string &query);

    // Raw byte API. INSERT / WRITE queries; returns bytes written.
    sb_size write(Transaction &tx, const void *src, b_size bytes, const std::string &query);

    // Typed API. read returns number of elements read.
    template <typename T>
    size_t read(Transaction &tx, T *dest, size_t count, const std::string &query) {
        detail::require_pod<T>();
        auto n = read(tx, static_cast<void *>(dest), count * sizeof(T), query);
        return static_cast<size_t>(n) / sizeof(T);
    }

    template <typename T>
    size_t read(Transaction &tx, std::vector<T> &dest, const std::string &query) {
        return read(tx, dest.data(), dest.size(), query);
    }

    template <typename T>
    sb_size write(Transaction &tx, const T *src, size_t count, const std::string &query) {
        detail::require_pod<T>();
        return write(tx, static_cast<const void *>(src), count * sizeof(T), query);
    }

    template <typename T>
    sb_size write(Transaction &tx, const std::vector<T> &src, const std::string &query) {
        return write(tx, src.data(), src.size(), query);
    }

    // READ / REMOVE everything the query yields into a vector (uses ns_malloc).
    template <typename T>
    std::vector<T> read_all(Transaction &tx, const std::string &query) {
        detail::require_pod<T>();
        b_size len = 0;
        void  *p   = malloc_raw(tx, &len, query);
        return detail::take_buffer<T>(p, len);
    }

private:
    void *malloc_raw(Transaction &tx, b_size *len, const std::string &query);
};

//////////////////////////////////// SmartFile

// Byte-addressed file. Offsets are in bytes; stride/count are in elements of T.
class SmartFile : public Handle {
public:
    explicit SmartFile(const std::string &path);

    sb_size size(Transaction &tx);

    // Insert (always bytes)
    sb_size insert(Transaction &tx, const void *src, b_size bytes, sb_size byte_offset);

    template <typename T>
    sb_size insert(Transaction &tx, const std::vector<T> &src, sb_size byte_offset) {
        detail::require_pod<T>();
        return insert(tx, src.data(), src.size() * sizeof(T), byte_offset);
    }

    // Append to the end of the file.
    template <typename T>
    sb_size append(Transaction &tx, const std::vector<T> &src) {
        return insert(tx, src, size(tx));
    }

    // Strided element access (raw)
    sb_size write_raw(Transaction &tx, const void *src, t_size elem_size,
                      sb_size byte_offset, sb_size stride, b_size count);
    sb_size read_raw(Transaction &tx, void *dest, t_size elem_size,
                     sb_size byte_offset, sb_size stride, b_size count);
    sb_size remove_raw(Transaction &tx, void *dest, t_size elem_size,
                       sb_size byte_offset, sb_size stride, b_size count);

    // Strided element access (typed)
    template <typename T>
    sb_size write(Transaction &tx, const T *src, size_t count,
                  sb_size byte_offset, sb_size stride = 1) {
        detail::require_pod<T>();
        return write_raw(tx, src, sizeof(T), byte_offset, stride, count);
    }

    template <typename T>
    sb_size write(Transaction &tx, const std::vector<T> &src,
                  sb_size byte_offset, sb_size stride = 1) {
        return write(tx, src.data(), src.size(), byte_offset, stride);
    }

    template <typename T>
    sb_size read(Transaction &tx, T *dest, size_t count,
                 sb_size byte_offset, sb_size stride = 1) {
        detail::require_pod<T>();
        return read_raw(tx, dest, sizeof(T), byte_offset, stride, count);
    }

    template <typename T>
    std::vector<T> read(Transaction &tx, size_t count,
                        sb_size byte_offset, sb_size stride = 1) {
        std::vector<T> out(count);
        read(tx, out.data(), count, byte_offset, stride);
        return out;
    }

    // Remove elements; pass dest = nullptr to discard the removed data.
    template <typename T>
    sb_size remove(Transaction &tx, T *dest, size_t count,
                   sb_size byte_offset, sb_size stride = 1) {
        detail::require_pod<T>();
        return remove_raw(tx, dest, sizeof(T), byte_offset, stride, count);
    }

    // Remove elements and return what was removed.
    template <typename T>
    std::vector<T> take(Transaction &tx, size_t count,
                        sb_size byte_offset, sb_size stride = 1) {
        std::vector<T> out(count);
        remove(tx, out.data(), count, byte_offset, stride);
        return out;
    }
};

} // namespace numstore

#endif // NUMSTORE_HPP
