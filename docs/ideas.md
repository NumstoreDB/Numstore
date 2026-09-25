Products:
=========

* pynumstore (Official Python Bindings)
    - A durable python array library that wraps numpy

```python
# Numstore
with ns.Database("example.db") as db:
    with db.begin() as tx:
        tx.execute("create prices [10][20]f64")

        removed = tx.execute("remove prices[0:]")

        tx.execute(f"insert prices 0 {src.shape[0]}", src)

        data = tx.execute("read prices[0:]")

# Smart files
with ns.Smartfile("example") as db:
    with db.begin() as tx:
        tx.insert("Hello world", 0)

        tx[0:].write("cat")

        data = tx[0:10].read()

        removed = tx[0:10]
```

* jnumstore (Official Java Bindings)
    - A wrapper
```java
# Numstore
try (Database db = Database.open("example.db"); Transaction tx = db.begin()) {
    try(Transaction tx = db.begin()) {
        tx.execute("create prices [10][20]f64");

        MemorySegment removed = tx.execute("remove prices[0:]").array();

        tx.execute("insert prices 0 %d".formatted(src.shape()[0]), src);

        MemorySegment data = tx.execute("read prices[0:]").array();
    }
}

# Smart files
try (SmartFile f = SmartFile.open("example");
     FileTransaction tx = f.begin()) {

    tx.insert("Hello world".getBytes(UTF_8), 0);

    tx.slice(0).write("cat".getBytes(UTF_8));        // tx[0:]
    byte[] data    = tx.slice(0, 10).read();         // tx[0:10].read()
    byte[] removed = tx.slice(0, 10).remove();       // tx[0:10] removed
    tx.slice(0, 100, 4).read();                      // tx[0:100:4]

    tx.commit();
}
```

1. Transactional Network file Server
    - A single server with the server protocol implemented
      that you can spin up
    - Frontend / Backend for data

2. Durable Java Arrays library
```
Durable
```

3. 
