import numpy as np
import numstore as ns

with ns.open("mydb") as db:
    counts = db.get_or_create("counts", dtype="i32")
    del counts[0:]

    with db.begin_txn() as txn:
        txn["counts"].append(np.array([1, 2, 3], dtype=np.int32))

    print("First state: ", counts[0:])

    try:
        with db.begin_txn() as txn:
            txn["counts"].append(np.array([99, 99, 99], dtype=np.int32))
            print("After modification (inside tx): ", txn["counts"][0:])
            raise RuntimeError("something went wrong")
    except RuntimeError:
        pass

    print("counts after rollback: ", db["counts"][0:])  
