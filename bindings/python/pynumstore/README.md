<p align="center">
  <img src="https://raw.githubusercontent.com/lincketheo/numstore/refs/heads/main/assets/logo.png" alt="NumStore Logo" width="200"/>
</p>

# PyNumstore

**An ACID database for numpy arrays**

Pynumstore is a database for numerical arrays. It's written in C from scratch with 
bindings in python. Pynumstore is fully ACID. Meaning you can pull the plug on your 
application and the database stays in a consistent state. 

---

1.0 Quick Start
===============

1.0 Install Numstore using pip
------------------------------
```
pip3 install pynumstore 
```

1.1 Run a Sample Application 
----------------------------

The following example shows the main operations you can do on a 
numstore database:

The available things you can do to a numstore database are:
*  **Create**       a new variable
*  **Delete**       a variable variable
*  **Get**          a variable 
*  **Insert**       data into a variable
*  **Read**         data from a variable
*  **Write**        data into a variable
*  **Remove**       data from a variable
*  **Begin**        a transation
*  **Commit**       an open transaction
*  **Rollback**     an open transaction
*  **Close**        and reopen a database

You can see more examples in samples/pynumstore

```
import numpy as np
import pynumstore as ns

# Basic Operations
with ns.open("mydb") as db:
    ######## Part 1 - A simple Create Insert Read Write Remove example
    y = db.get_or_create("y", dtype="f32")

    # Start with a fresh dataset if y already existed
    del y[0:]
    print("Initial State: ", y[0:])

    # Append twice
    y.append(np.array([1.0, 2.0, 3.0], dtype=np.float32))
    y.append(np.array([4.0, 5.0, 6.0], dtype=np.float32))

    # Retrieve the whole array
    print("Seed Data: ", y[0:])

    # Insert in the middle (inner mutations are first class operations)
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

    ######## Part 2 - Demonstrating some ACID properties of numstore

    # Without a transaction - mutations happen in "auto transaction mode"
    counts = db.get_or_create("counts", dtype="i32")
    del counts[0:]

    # You can wrap operations inside a transaction to be explicit
    with db.begin_txn() as txn:
        txn["counts"].append(np.array([1, 2, 3], dtype=np.int32))

    print("First state: ", counts[0:])

    # When an exception is thrown - numstore roll's back changes:
    try:
        with db.begin_txn() as txn:
            txn["counts"].append(np.array([99, 99, 99], dtype=np.int32))
            print("After modification (inside tx): ", txn["counts"][0:])
            raise RuntimeError("something went wrong")
    except RuntimeError:
        pass

    print("counts after rollback: ", db["counts"][0:])  
```

2.0 License
============

Apache 2.0. See LICENSE.
