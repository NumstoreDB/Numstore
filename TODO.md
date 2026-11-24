Big Features
============

Directory Model Refactor 
------------------------
- create foo u32

Fine grained concurrency
------------------------
- Feature flag EXPERIMENTAL_HIERARCHIAL_TXNS
- Problems:
    - Currently the WAL does full physical page logging 
      if you don't supply a custom log. 
      That's fine when you lock transactions but
    - Locking free space map will be tricky

Testing and Reliability
-----------------------
- Goal - 100% branch coverage in unit tests 
- Large categories for tedious tests:
    - Arithmetic wrapping 
    - Testing error paths
    - Concurrency tests
    - Making ns_simtest concurrent (fixing AI slop)
- Merging tests with duplicate behavior 

Locking an open database 
-------------------------
- Make it so that two processes can't open the same database 
  I think using an active pid in the database header is one idea
  but the atomicity needs to be evaluated here

Optimization and niceties:
--------------------------
- Database doesn't need to be so big early on 
    - Right now every time it allocates a new free space map,
      it just allocates the entire chain
- Inserts can allocate large swaths of pages rather than 
  doing pgr_new for every change

Database as a file format 
--------------------------
- Database should be portable across systems

Documentation and onboarding
----------------------------
- Numstore website
- better docs and pandoc generation
- sample apps

First class rust interior integration
-------------------------------------
- numstore-rs is the rust wrapper (exterior - not interior)
- Goal:
    - have one library in libs fully swapped out for a rust 
      one 
- Challenges:
    - Interior axiomatic code like i_mem and i_fs
      I think just wrap those in rust is fine

Smaller Features 
================

- Better latch design
    - Not optimized for thread cache lines
