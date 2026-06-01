import numpy as np
import pynumstore as ns

# Overwriting existing entries
with ns.open("mydb") as db:
    v = db.get_or_create("signal", dtype="f32")
    del v[0:]

    v.append(np.array([0.0, 0.0, 0.0], dtype=np.float32))
    v.append(np.array([0.0, 0.0, 0.0], dtype=np.float32))
    v.append(np.array([0.0, 0.0, 0.0], dtype=np.float32))

    # Overwrite data at the start
    v[1:4] = np.array([9.0, 9.0, 9.0], dtype=np.float32)
    print(v[0:])

    # Overwrite data at the start
    v[3:7] = np.array([1, 2, 3, 4], dtype=np.float32)
    print(v[0:]) 

