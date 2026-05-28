from .ns_to_np import *
from ._numstore import *
from ._numstore import var_create as _c_var_create
from ._db_wrapper import Database, Transaction, Variable


def open(path) -> Database:
    """Context manager that opens a database and closes it on exit."""
    return Database(path)

__all__ = [
    "open",
    "Transaction",
    "Variable",
    "ns_to_np",
]
