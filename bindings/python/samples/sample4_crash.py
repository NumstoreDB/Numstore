"""durability across simulated process crash"""
import os
import numpy as np
import pynumstore as ns

PATH = "durability.db"

def phase1_populate():
    with ns.Database(PATH) as db:
        db.execute("create log u8")
        db.execute("insert log 0 10", np.frombuffer(b"AAAAAAAAAA", dtype=np.uint8))


def phase2_commit_then_crash():
    db = ns.Database(PATH)
    txn = db.begin()
    txn.execute("insert log 3 2", np.frombuffer(b"BB", dtype=np.uint8))
    txn.commit()
    os._exit(1)  # simulate crash - this data is durable because commit was called


def phase3_crash():
    db = ns.Database(PATH)
    txn = db.begin()
    txn.execute("insert log 7 2", np.frombuffer(b"CC", dtype=np.uint8))
    os._exit(1)  # simulate crash - this data is not durable because commit was never called 


def run_in_child(fn) -> None:
    """Run fn() in a forked child process and wait for it to finish.

    Fork lets each phase get its own isolated process, the same way the C
    sample re-execs itself per phase - phases that crash never return
    control to this process's own Database handle.
    """
    pid = os.fork()
    if pid == 0:
        fn()
        os._exit(0)  
    os.waitpid(pid, 0)


def show(label: str, data: np.ndarray) -> None:
    print(f"{label}: {bytes(data).decode()!r}")


if __name__ == "__main__":
    if os.path.exists(PATH):
        os.remove(PATH)

    run_in_child(phase1_populate)
    run_in_child(phase2_commit_then_crash)
    run_in_child(phase3_crash)

    # Final step
    with ns.Database(PATH) as db:
        show("full contents", db.execute("read log[0:]"))
        show("phase 2 - commit + crash      (expect BB)", db.execute("read log[3:5]"))
        show("phase 3 - no commit + crash   (expect AA)", db.execute("read log[7:9]"))
