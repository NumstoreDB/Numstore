import numpy as np
import pynumstore as ns

# Transactions — commit and rollback
with ns.open("mydb") as db:
    counts = db.var("counts", dtype="i32", create=True)
    del counts[0:]

    # Committed transaction — changes persist
    with db.begin_txn() as txn:
        txn["counts"].append(np.array([1, 2, 3], dtype=np.int32))

    # Rolled-back transaction — changes are discarded
    try:
        with db.begin_txn() as txn:
            txn["counts"].append(np.array([99, 99, 99], dtype=np.int32))
            raise RuntimeError("something went wrong")
    except RuntimeError:
        pass

    print(db["counts"][0:])  # only [1, 2, 3]
