# R bindings for numstore.
#
# Element types (the `type` argument) are numstore type names:
#   "i8", "u8", "i16", "u16", "i32"  -> returned as R integer vectors
#   "u32", "i64", "u64", "f32", "f64" -> returned as R double vectors
# 64-bit integers are exact only up to 2^53, because R stores them as doubles.
#
# All handles (databases, smart files, transactions, plans) are external
# pointers: they are released by the garbage collector, but cannot be saved
# with save()/saveRDS() and reloaded.

ns_types <- c("i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "f32", "f64")

.check_type <- function(type) {
  if (missing(type))
    stop("`type` is required; one of: ", paste(ns_types, collapse = ", "), call. = FALSE)
  match.arg(type, ns_types)
}

.need <- function(x, cls, arg) {
  if (!inherits(x, cls))
    stop(sprintf("`%s` must be a %s object", arg, cls), call. = FALSE)
  invisible(x)
}

.tag <- function(ptr, cls, ...) {
  attrs <- list(...)
  for (nm in names(attrs)) attr(ptr, nm) <- attrs[[nm]]
  class(ptr) <- cls
  ptr
}

#################################### Lifecycle

#' Open a numstore database.
ns_open <- function(path) {
  path <- path.expand(path)
  .tag(cpp_open(path), c("numstore_db", "numstore_handle"), path = path)
}

#' Open a numstore smart file.
smf_open <- function(path) {
  path <- path.expand(path)
  .tag(cpp_smfile_open(path), c("numstore_smfile", "numstore_handle"), path = path)
}

#' Close a database or smart file. Open transactions are rolled back and
#' plans are freed first.
ns_close <- function(handle) {
  .need(handle, "numstore_handle", "handle")
  cpp_close(handle)
  invisible(NULL)
}

#' Harshly close a database or smart file. Incomplete transactions are
#' rolled back the next time it is opened.
ns_crash <- function(handle) {
  .need(handle, "numstore_handle", "handle")
  cpp_crash(handle)
  invisible(NULL)
}

#' Delete the numstore database or smart file at `path`.
ns_remove <- function(path) {
  cpp_cleanup(path.expand(path))
  invisible(NULL)
}

ns_is_open <- function(handle) {
  .need(handle, "numstore_handle", "handle")
  cpp_is_open(handle)
}

ns_last_error <- function(handle) {
  .need(handle, "numstore_handle", "handle")
  cpp_last_error(handle)
}

#################################### Transactions

ns_begin <- function(handle) {
  .need(handle, "numstore_handle", "handle")
  .tag(cpp_begin(handle), "numstore_txn")
}

ns_commit <- function(tx) {
  .need(tx, "numstore_txn", "tx")
  cpp_commit(tx)
  invisible(NULL)
}

ns_rollback <- function(tx) {
  .need(tx, "numstore_txn", "tx")
  cpp_rollback(tx)
  invisible(NULL)
}

ns_txn_active <- function(tx) {
  .need(tx, "numstore_txn", "tx")
  cpp_txn_active(tx)
}

#' Run `fn(tx)` inside a transaction. Commits if `fn` returns normally and
#' rolls back if it signals an error. Returns whatever `fn` returns.
ns_transaction <- function(handle, fn) {
  tx <- ns_begin(handle)
  on.exit(if (cpp_txn_active(tx)) cpp_rollback(tx), add = TRUE)
  result <- withVisible(fn(tx))
  if (cpp_txn_active(tx)) cpp_commit(tx)
  if (result$visible) result$value else invisible(result$value)
}

#################################### Database queries

#' Run a query that takes no data, e.g. "create foo u32" or "delete foo".
ns_execute <- function(tx, query) {
  .need(tx, "numstore_txn", "tx")
  cpp_execute(tx, query)
  invisible(NULL)
}

#' Length (in elements) of the variable a query refers to, e.g. "get foo".
ns_var_len <- function(tx, query) {
  .need(tx, "numstore_txn", "tx")
  cpp_var_len(tx, query)
}

#' Run a READ / REMOVE query, e.g. "read foo[0:10]", and return the values.
ns_read <- function(tx, query, type) {
  .need(tx, "numstore_txn", "tx")
  cpp_read(tx, query, .check_type(type))
}

#' Run an INSERT / WRITE query, e.g. "insert foo 0 10", with `values`.
#' Returns the number of bytes written, invisibly.
ns_write <- function(tx, query, values, type) {
  .need(tx, "numstore_txn", "tx")
  invisible(cpp_write(tx, query, values, .check_type(type)))
}

#################################### Plans

#' Prepare a query. Plans are freed automatically when the database closes.
ns_plan <- function(db, query) {
  .need(db, "numstore_db", "db")
  .tag(cpp_plan(db, query), "numstore_plan", query = query)
}

ns_plan_execute <- function(plan, tx) {
  .need(plan, "numstore_plan", "plan")
  .need(tx, "numstore_txn", "tx")
  cpp_plan_execute(plan, tx)
  invisible(NULL)
}

ns_plan_var_len <- function(plan, tx) {
  .need(plan, "numstore_plan", "plan")
  .need(tx, "numstore_txn", "tx")
  cpp_plan_var_len(plan, tx)
}

ns_plan_read <- function(plan, tx, type) {
  .need(plan, "numstore_plan", "plan")
  .need(tx, "numstore_txn", "tx")
  cpp_plan_read(plan, tx, .check_type(type))
}

ns_plan_write <- function(plan, tx, values, type) {
  .need(plan, "numstore_plan", "plan")
  .need(tx, "numstore_txn", "tx")
  invisible(cpp_plan_write(plan, tx, values, .check_type(type)))
}

ns_plan_free <- function(plan) {
  .need(plan, "numstore_plan", "plan")
  cpp_plan_free(plan)
  invisible(NULL)
}

#################################### Smart files
# Offsets are in bytes. `n` and `stride` are in elements of `type`.

#' Size of the smart file in bytes.
smf_size <- function(tx) {
  .need(tx, "numstore_txn", "tx")
  cpp_smf_size(tx)
}

#' Insert `values` at byte `offset`, shifting later data right.
smf_insert <- function(tx, values, offset, type) {
  .need(tx, "numstore_txn", "tx")
  invisible(cpp_smf_insert(tx, values, .check_type(type), offset))
}

#' Insert `values` at the end of the file.
smf_append <- function(tx, values, type) {
  smf_insert(tx, values, smf_size(tx), type)
}

#' Overwrite elements starting at byte `offset`, every `stride` elements.
smf_write <- function(tx, values, offset, type, stride = 1) {
  .need(tx, "numstore_txn", "tx")
  invisible(cpp_smf_write(tx, values, .check_type(type), offset, stride))
}

#' Read `n` elements starting at byte `offset`, every `stride` elements.
smf_read <- function(tx, n, offset = 0, type, stride = 1) {
  .need(tx, "numstore_txn", "tx")
  cpp_smf_read(tx, n, .check_type(type), offset, stride)
}

#' Remove `n` elements starting at byte `offset`, every `stride` elements.
#' Returns the removed values (or NULL invisibly when `keep = FALSE`).
smf_remove <- function(tx, n, offset, type, stride = 1, keep = TRUE) {
  .need(tx, "numstore_txn", "tx")
  out <- cpp_smf_remove(tx, n, .check_type(type), offset, stride, keep)
  if (keep) out else invisible(NULL)
}

#################################### Printing

print.numstore_db <- function(x, ...) {
  cat(sprintf("<numstore database %s (%s)>\n", attr(x, "path"),
              if (cpp_is_open(x)) "open" else "closed"))
  invisible(x)
}

print.numstore_smfile <- function(x, ...) {
  cat(sprintf("<numstore smart file %s (%s)>\n", attr(x, "path"),
              if (cpp_is_open(x)) "open" else "closed"))
  invisible(x)
}

print.numstore_txn <- function(x, ...) {
  cat(sprintf("<numstore transaction (%s)>\n",
              if (cpp_txn_active(x)) "active" else "finished"))
  invisible(x)
}

print.numstore_plan <- function(x, ...) {
  cat(sprintf("<numstore plan: %s>\n", attr(x, "query")))
  invisible(x)
}
