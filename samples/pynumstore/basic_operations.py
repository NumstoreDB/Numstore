import numpy as np
import pynumstore as ns

# Basic Operations
with ns.open("mydb") as db:
    with db.begin_txn() as txn:

        y = txn.var("y", dtype="f32", create=True)

        # Start with a fresh dataset if y already existed
        del y[0:]
        print(y[0:])

        """
        # Append twice
        y.append(np.array([1.0, 2.0, 3.0], dtype=np.float32))
        y.append(np.array([4.0, 5.0, 6.0], dtype=np.float32))

        # Retrieve the whole array
        print(y[0:])

        # Insert in the middle
        y.insert(2, np.array([4.0, 5.0, 6.0], dtype=np.float32))

        # Retrieve the whole array
        print(y[0:])

        # Remove every even index
        del y[0::2]
        print(y[0:])
        """
