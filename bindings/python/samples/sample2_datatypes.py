"""Showing numstores type system"""
import numpy as np
import pynumstore as ns

# A Numstore type
datatype = """
struct {
    id u32,
    name [10]u8,
    height u32
}
"""

# Convert it to a numpy type
record_dtype = ns.to_dtype(datatype)
print(record_dtype)

# Build 10 starting records, as a plain list of row tuples
names = ["alice", "bob", "cid", "dana", "eve", "frank", "gina", "hank", "ivy", "jack"]
src = np.array(
    [
        (i + 1, tuple(name.encode().ljust(10, b"\x00")), 150 + i)
        for i, name in enumerate(names)
    ],
    dtype=record_dtype,
)

# More Data to insert later
more = np.array(
    [(11, tuple(b"kim".ljust(10, b"\x00")), 174)],
    dtype=record_dtype,
)

def print_records(arr: np.ndarray, title: str = "Records") -> None:
    """Pretty-print a structured array as a simple table, decoding the
    raw `name` byte array back to a string for display.
    """
    print(title)
    header = "  ".join(f"{name:<10}" for name in arr.dtype.names)
    print(header)
    print("-" * len(header))
    for id_, name_bytes, height in arr:
        name_str = bytes(name_bytes).rstrip(b"\x00").decode()
        print(f"{id_:<10}  {name_str:<10}  {height:<10}")
    print()


with ns.Database("example.db") as db:
    # Create a new variable using the struct/array type
    db.execute(f"create readings {datatype}")  

    # Delete everything if it exists
    db.execute("remove readings[0:]")

    # Insert data into index 0
    db.execute(f"insert readings 0 {src.size}", src)

    # Read the data we wrote
    dest = db.execute("read readings[0:]")
    print_records(dest, "readings[0:]")

    # Insert another record starting at index 10
    db.execute(f"insert readings 2 {more.size}", more)

    # Read every record back
    dest = db.execute("read readings[0:]")
    print_records(dest, "readings[0:] (after insert at index 2)")

    # Delete every 3rd record
    removed = db.execute("remove readings[0::3]")
    print_records(removed, "removed readings[0::3] (these are the values that were removed)")

    # Read what's left
    dest = db.execute("read readings[0:]")
    print_records(dest, "remaining after remove")
