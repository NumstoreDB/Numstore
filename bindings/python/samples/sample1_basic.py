"""Basic operations on an array of f64's"""
import numpy as np

import faulthandler
faulthandler.enable()

import pynumstore as ns
import os

with ns.Database("example.db") as db:
    # Create a new f64 variable
    db.execute("create prices f64")

    # Delete everything if it exists
    db.execute("remove prices[0:]")

    # Insert data into index 0
    src = np.array([1.5, 2.25, 3.75], dtype=np.float64)
    db.execute(f"insert prices 0 {src.size}", src)

    # Read the data we wrote
    dest = db.execute(f"read prices[0:]")
    print("Prices: ", dest)

    # Insert data starting at index 2
    src = np.array([4.5, 9.25, 10.75], dtype=np.float64)
    db.execute(f"insert prices 2 {src.size}", src)

    # Read every 2nd element of the array
    dest = db.execute(f"read prices[0:]")
    print("Prices: ", dest)

    # Delete every 3rd element of the array
    removed = db.execute(f"remove prices[0::3]")
    print("Removed: ", removed)

    # Read every 2nd element of the array
    dest = db.execute(f"read prices[0:]")
    print("Remaining: ", dest)
