from typing import Any, NewType

import numpy as np

from typing_extensions import Buffer, CapsuleType

nsdb = NewType("nsdb", CapsuleType)
nstxn = NewType("nstxn", CapsuleType)
nsvar = NewType("nsvar", CapsuleType)
nsplan = NewType("nsplan", CapsuleType)

# ---------------------------------------------------------------- Types

def _pyns_ns_to_np(s: str, /) -> np.dtype[Any]:
def _pyns_ns_to_np_flatten(s: str, n: int, /) -> tuple[tuple[int, ...], np.dtype[Any]]:

# ---------------------------------------------------------------- Lifecycle

def _pyns_open(path: str, /) -> nsdb:
def _pyns_close(db: nsdb, /) -> None:

# ---------------------------------------------------------------- Transactions

def _pyns_begin(db: nsdb, /) -> nstxn:
def _pyns_commit(db: nsdb, txn: nstxn, /) -> None:
def _pyns_rollback(db: nsdb, txn: nstxn, /) -> None:

# ---------------------------------------------------------------- Plans
#
# A plan is a compiled query that can be run many times. Passing txn=None
# runs it in an automatic transaction.

def _pyns_plan_create(db: nsdb, query: str, /) -> nsplan:
def _pyns_plan_close(plan: nsplan, /) -> None:
def _pyns_plan_execute(plan: nsplan, txn: nstxn | None, /) -> int:
def _pyns_plan_get_var(plan: nsplan, txn: nstxn | None, /) -> nsvar:
def _pyns_plan_read(plan: nsplan, txn: nstxn | None, dest: Buffer, /) -> int:
def _pyns_plan_malloc(plan: nsplan, txn: nstxn | None, /) -> bytes:
def _pyns_plan_write(plan: nsplan, txn: nstxn | None, src: Buffer, /) -> int:

# ---------------------------------------------------------------- Variables
#
# Variables own their memory and outlive the database they came from.

def _pyns_var_free(var: nsvar, /) -> None:
def _pyns_var_name(var: nsvar, /) -> str: ...
def _pyns_var_length(var: nsvar, /) -> int:
def _pyns_var_tsize(var: nsvar, /) -> int:
def _pyns_var_type(var: nsvar, /) -> str:
