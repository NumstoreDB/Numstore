"""Example: every documented error case, triggered and caught explicitly."""
import numpy as np
import pynumstore as ns

PATH = "errors.db"

def show(label: str, fn) -> None:
    try:
        fn()
    except Exception as e:
        print(f"{label}: {type(e).__name__}: {e}")
    else:
        print(f"{label}: no error raised")


with ns.Database(PATH) as db:
    show("Invalid dtype", lambda: db.execute("create foo struct { a u32"))
    db.execute("create foo u32")

    show("(insert) Missing array length", lambda: db.execute("insert foo 0"))
    show("(insert) Invalid parse", lambda: db.execute("insert 0 1"))
    show("(insert) Missing array", lambda: db.execute("insert foo 0 1"))
    show("(read) Malformed query", lambda: db.execute("read bar[0"))
    show("(read) Nonexistent variable", lambda: db.execute("read bar[0:]"))

    txn = db.begin()
    txn.commit()
    show(
        "closed transaction reused",
        lambda: txn.execute("read foo[0:]", np.zeros(1, dtype=np.uint32)),
    )

# Using the database after it's closed -> RuntimeError
db2 = ns.Database(PATH)
db2.close()
show("closed database reused", lambda: db2.execute("create baz u32"))

# Double close is a no-op, not an error
show("double close", lambda: db2.close())
