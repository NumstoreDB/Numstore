import pynumstore as ns

with ns.Database("prices.db") as db:
    # Make a variable. Its elements are f64 numbers.
    db.execute("create prices f64")

    # Start with no data
    db.execute("remove prices[0:]")

    # Insert 3 numbers at index 0
    db.write("insert prices 0 3", [1.5, 2.25, 3.75])

    # Insert 1 number in the middle
    db.write("insert prices 1 1", [9.0])

    # Read all of the numbers
    print(db.read("read prices").tolist())
    # [1.5, 9.0, 2.25, 3.75]
