import numpy as np
import numstore as ns

# Basic Operations
with ns.open("mydb") as db:
    with db.begin_txn() as txn:

        y = txn.var("y", dtype="f32", create=True)

        # Start with a fresh dataset if y already existed
        del y[0:]
        print("Initial State: ", y[0:])

        # Append twice
        y.append(np.array([1.0, 2.0, 3.0], dtype=np.float32))
        y.append(np.array([4.0, 5.0, 6.0], dtype=np.float32))

        # Retrieve the whole array
        print("Seed Data: ", y[0:])

        # Insert in the middle
        y.insert(2, np.array([4.0, 5.0, 6.0], dtype=np.float32))

        # Retrieve the whole array
        print("After inner insert: ", y[0:])

        # Overwrite data at the start
        y[1:4] = np.array([9.0, 9.0, 9.0], dtype=np.float32)
        print("After overwrite 1: ", y[0:])

        # Overwrite data at the start
        y[3:7] = np.array([1, 2, 10, 12], dtype=np.float32)
        print("After overwrite 2: ", y[0:]) 

        # Remove every even index
        del y[0::2]
        print("End state: ", y[0:])
