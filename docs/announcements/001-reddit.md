# Announcing Pynumstore

Hi all - I'm releasing pynumstore, which is a fully ACID database for numpy
arrays.

To try it out:
```
git clone https://github.com/NumstoreDB/Numstore 
cd Numstore 
make python-package
pip3 install build/python/target/*.whl --force-reinstall
python3 bindings/python/samples/sample1_basic.py 
```

Don't do `pip3 install pynumstore` (yet) - I published an older beta version to
pypi I don't have the time to get a good cross compiled version on pypi - so
building from source is the most reliable.

It's primarily a C library - I hastily wrote python bindings in a couple of
days with the help of AI - but it was meant to store numpy - like arrays. 

It's a single file, embedded database. For those who are interested in the
internals:

1. It uses ARIES for crash recovery and transaction support
2. It's entirely in process - so 2 processes can't talk to it at the same time
   (I'm thinking about how to change this - just don't have any users to drive
   my direction yet)
3. It uses a novel variant of the B+Tree that I'm calling the Rope+Tree. It's
   internals are similar to a B+Tree, but it uses array length as an index
   instead of explicit keys 
4. It uses a paged approach using a steal no force buffer pool
5. Concurrency is a work in progress. Transactions are entirely serializable,
   so one transaction happens at a time. It doesn't have to be like this, I
   have 2 phase locking infrastructure, I just don't trust it enough. This will
   definitely change in my next milestone. Concurrency is a big focus of mine.
   You just might deal with some deadlocks early on
6. There's a novel language I designed around structs, unions, arrays and
   primitives. You can read about it
   [here](https://github.com/NumstoreDB/Numstore/tree/develop/docs)
7. It compiles on mac linux and windows, but I'm not good at supporting
   windows, so Posix is probably more reliable.
