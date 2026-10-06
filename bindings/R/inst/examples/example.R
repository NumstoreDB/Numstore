library(numstore)

db <- ns_open("test.db")

# ns_transaction commits on success, rolls back if the function errors
ns_transaction(db, function(tx) {
  ns_execute(tx, "create foo u32")
  ns_write(tx, "insert foo 0 10", 0:9, type = "u32")
})

# Or manage the transaction yourself
tx <- ns_begin(db)
cat("foo has", ns_var_len(tx, "get foo"), "elements\n")
print(ns_read(tx, "read foo[0:10]", type = "u32"))
ns_commit(tx)

# Prepared plans
p <- ns_plan(db, "read foo[0:5]")
first <- ns_transaction(db, function(tx) ns_plan_read(p, tx, type = "u32"))
print(first)

ns_close(db)  # rolls back open transactions and frees plans first

# Smart files: offsets are in bytes, n / stride in elements
smf <- smf_open("test.smf")
ns_transaction(smf, function(tx) {
  smf_append(tx, 0:5, type = "u32")
  print(smf_read(tx, 3, offset = 4, type = "u32", stride = 2))  # 1 3 5
})
ns_close(smf)
