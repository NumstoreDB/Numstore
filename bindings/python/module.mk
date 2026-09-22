############ Module entry point and database lifecycle

ALL_PYSRCS += bindings/python/src/c/ns_pynumstore.c
ALL_PYSRCS += bindings/python/src/c/ns_pyns_open.c
ALL_PYSRCS += bindings/python/src/c/ns_pyns_close.c

############ Types (numstore type <-> numpy dtype)

ALL_PYSRCS += bindings/python/src/c/types/ns_pyns_dims.c
ALL_PYSRCS += bindings/python/src/c/types/ns_pyns_ns_to_np.c
ALL_PYSRCS += bindings/python/src/c/types/ns_pyns_type_to_dtype.c
ALL_PYSRCS += bindings/python/src/c/types/ns_pyns_type_to_dtype_flatten_sarray.c

############ Transactions

ALL_PYSRCS += bindings/python/src/c/txn/ns_pyns_begin.c
ALL_PYSRCS += bindings/python/src/c/txn/ns_pyns_commit.c
ALL_PYSRCS += bindings/python/src/c/txn/ns_pyns_rollback.c

############ Execution

ALL_PYSRCS += bindings/python/src/c/execute/ns_pyns_execute.c
ALL_PYSRCS += bindings/python/src/c/execute/ns_pyns_execute_data_present.c
ALL_PYSRCS += bindings/python/src/c/execute/ns_pyns_execute_data_not_present.c

############ Variables

ALL_PYSRCS += bindings/python/src/c/var/ns_pyns_var_capsule.c
ALL_PYSRCS += bindings/python/src/c/var/ns_pyns_var_length.c
ALL_PYSRCS += bindings/python/src/c/var/ns_pyns_var_name.c
ALL_PYSRCS += bindings/python/src/c/var/ns_pyns_var_tsize.c
ALL_PYSRCS += bindings/python/src/c/var/ns_pyns_var_type.c
