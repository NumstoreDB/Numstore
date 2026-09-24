from typing import Any

import numpy as np
import numpy.typing as npt

type nsdb = Any
type nstxn = Any
type nsvar = Any

# Convert a numstore type (a string) to a numpy dtype
def pyns_ns_to_np(s: str) -> np.dtype[Any]: ...

# Open and close a database
def pyns_open(path: str) -> nsdb: ...
def pyns_close(db: nsdb) -> None: ...

# Transaction control
def pyns_begin(db: nsdb) -> nstxn: ...
def pyns_commit(db: nsdb, txn: nstxn) -> None: ...
def pyns_rollback(db: nsdb, txn: nstxn) -> None: ...

# The main method of execution
#   db: The database connection to run on
#   txn: An open transaction or none for auto transaction
#   query: The query to run
#   data: source (insert/write) or destination (read/remove) buffer, or None
#
# With `data` given, the element count comes back. With `data` None the result
# is whatever the query produced: an array when it allocated one (read,
# remove), a capsule owning a numstore_var when it resolved a variable (get),
# and None when it produced neither (create, delete). The capsule releases the
# variable itself once the last reference to it goes away.
def pyns_execute(
    db: nsdb, txn: nstxn | None, query: str, data: npt.NDArray[Any] | None
) -> int | npt.NDArray[Any] | nsvar | None: ...

# Thin accessors over a captured variable capsule. The capsule's destructor
# releases the variable when the last reference to it goes away - there is no
# free to call.
def pyns_var_name(var: nsvar) -> str: ...
def pyns_var_length(var: nsvar) -> int: ...
def pyns_var_tsize(var: nsvar) -> int: ...
def pyns_var_type(var: nsvar) -> str: ...
