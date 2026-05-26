import numpy as np
import pynumstore as ns

# Multiple variables with different dtypes
with ns.open("mydb") as db:
    # Create two variables
    timestamps = db.var("timestamps", dtype="f64", create=True)
    labels = db.var("labels", dtype="i32", create=True)

    with db.begin_txn() as txn:
        del txn["timestamps"][0:]
        del txn["labels"][0:]

    with db.begin_txn() as txn:
        txn["timestamps"].append(
                np.array([1.0, 1.5, 2.0], dtype=np.float64)
        )

        txn["labels"].append(
                np.array([0, 1, 0], dtype=np.int32)
        )

    print(db["timestamps"][0:])
    print(db["labels"][0:])
