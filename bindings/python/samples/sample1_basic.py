"""Basic operations on an array of [10][20]f64's"""
import numpy as np

import pynumstore as ns

rng = np.random.default_rng()


def show(label, arr):
    # print the first element of each of the N grids
    firsts = [f"{v:.3g}" for v in arr[:, 0, 0]]
    print(f"{label} shape={arr.shape} arr[0][0] = [{', '.join(firsts)}]")


with ns.Database("example.db") as db:
    # Create a new variable whose element is a 10x20 block of f64's
    db.execute("create prices [10][20]f64")

    # Delete everything if it exists
    db.execute("remove prices[0:]")

    # Insert 3 elements at index 0. 
    src = rng.random((3, 10, 20), dtype=np.float64)
    db.execute(f"insert prices 0 {src.shape[0]}", src)

    # Read the data we wrote
    dest = db.execute("read prices[0:]")
    show("Prices", dest)

    # Insert 3 more elements starting at index 2
    src = rng.random((3, 10, 20), dtype=np.float64)
    db.execute(f"insert prices 2 {src.shape[0]}", src)

    # Read the whole array back
    dest = db.execute("read prices[0:]")
    show("Prices", dest)

    # Delete every 3rd element of the array
    removed = db.execute("remove prices[0::3]")
    show("Removed", removed)

    # Read what's left
    dest = db.execute("read prices[0:]")
    show("Remaining", dest)

    var = db.execute("get prices")
    print(var)
