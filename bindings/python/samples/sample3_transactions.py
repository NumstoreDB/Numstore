"""transactions and rollbacks"""
import numpy as np
import pynumstore as ns


def show(label: str, arr: np.ndarray) -> None:
    print(label)
    print(arr)

with ns.Database("example.db") as db:
    db.execute("create events u32")

    # Delete all data
    db.execute("remove events[0:]")

    # Normal transaction with commit
    with db.begin() as txn:
        txn.execute("insert events 0 3", np.array([1, 2, 3], dtype=np.uint32))
    show("Committed", db.execute("read events[0:]"))

    # Transaction with manual rollback
    with db.begin() as txn:
        txn.execute("insert events 1 2", np.array([4, 5], dtype=np.uint32))
        show("Inside txn before rollback", txn.execute("read events[0:]"))
        txn.rollback()
    show("Explicit rollback", db.execute("read events[0:]"))

    # Transaction with exception based rollback
    try:
        with db.begin() as txn:
            txn.execute("insert events 1 2", np.array([9, 9], dtype=np.uint32))
            show("Inside txn before failure", txn.execute("read events[0:]"))
            raise RuntimeError("simulated validation failure")
    except RuntimeError:
        pass
    show("Exception rollback", db.execute("read events[0:]"))

    # Final committed transaction
    with db.begin() as txn:
        txn.execute("insert events 3 2", np.array([6, 7], dtype=np.uint32))
    show("Clean transaction", db.execute("read events[0:]"))
