# Getting Started

Everything below is a real Python session. Install the bindings:

```sh
pip install pynumstore
```

And open a database. A Numstore database is a single file, and there's no
server to start:

```python
>>> import numpy as np
>>> import pynumstore as ns
>>> db = ns.Database("tour.db")
>>> db
<Database 'tour.db'>
>>> db.path
'tour.db'
```

`Database` is a context manager, so in a script you'd normally write
`with ns.Database("tour.db") as db:` and let the block close it. In the REPL
it's easier to keep `db` around and call `db.close()` when you're done.

If you want to follow along, type the examples in order - each section builds on
the one before it.
