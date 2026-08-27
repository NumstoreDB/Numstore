from typing import Any

import numpy as np
import numpy.typing as npt

type nsdb = Any
type nstxn = Any

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
#   data: source (insert/write) or destination (read/remove) buffer, or
#         None - for a read/remove, None allocates and returns an array
#         instead of an element count
def pyns_execute(
    db: nsdb, txn: nstxn | None, query: str, data: npt.NDArray[Any] | None
) -> int | npt.NDArray[Any]: ...
