import numpy as np
import numstore as ns

# Persistence — data survives close and reopen
with ns.open("mydb") as db:
    log = db.get_or_create("log", dtype="i32")

    # Delete all the data if log already exists
    del log[0:]
    log.append(np.array([10, 20, 30], dtype=np.int32))

# Reopen the database and read it all
with ns.open("mydb") as db:
    print(db["log"][0:])  
