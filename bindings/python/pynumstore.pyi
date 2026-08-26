from typing import Any

import numpy as np
import numpy.typing as npt

type nsdb = Any
type nstxn = Any

# Convert a numstore type (a string) to a numpy dtype
def ns_to_np(s: str) -> np.dtype: ...

# Open and close a database
def pyns_open(path: str) -> nsdb: ...
def pyns_close(db: nsdb) -> None: ...

# Transaction control
def pyns_begin(db: nsdb) -> nstxn: ...
def pyns_commit(txn: nstxn) -> None: ...
def pyns_rollback(txn: nstxn) -> None: ...

# The main method of execution
#   db: The database connection to run on
#   txn: An open transaction or none for auto transaction
#   query: The query to run
#   data: Any data to provide to the query
def pyns_execute(
        db: nsdb, 
        txn: nstxn | None, 
        query: str, 
        data: np.array | None
) -> Any: ...
