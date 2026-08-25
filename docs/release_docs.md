Numstore Release Docs
=====================

Layout 
------

`bin` - Contains pre compiled binaries, tools, and samples
```
bin/
    # The main numstore cli tool (A wip repl)
    numstore                       

    # Tools for debugging a numstore database
    print_query                    
    dlread                         
    nsspprint                      
    nspprint                       
    print_type                     
    resolve_type_ref               
    walpprint

    # Compiled samples (see samples/*)
    smfile_sample2_transactions
    smfile_sample1_basic_crud
    ns_sample1_basic_crud          
    smfile_sample3_stride
    smfile_sample4_rollback_commit
```

`include` - Contains numstore and smartfiles headers
```
include/
    # A database for numerical arrays 
    numstore.h

    # A database for just bytes 
    smartfiles.h
```

`lib` - Contains the numstore library
```
lib/
    # Compile against this to use numstore
    libnumstore.a 
```

`samples` - Contains source sample files
```
samples/
    # Numstore samples
    ns_....c 

    # Smartfiles samples
    smfile_...c 
```


Compiling against the Numstore library
--------------------------------------

Just link against `libnumstore.a` and include the `include` directory 
```
$ gcc samples/ns_sample1_basic_crud.c -I include -Llib -lnumstore
```

Using the Numstore REPL 
-----------------------

This is a work in progress

```
$ ./bin/numstore mydb.db
numstore> create a u32;
{ "Status" : "Ok" }
numstore> create b u32;
{ "Status" : "Ok" }
numstore> get a;
{
    "Status" : "Ok",
    "Name"   : "a",
    "DType"  : "u32",
    "DSize"  : 4,
    "Nelems" : 0,
    "Bytes"  : 0,
    "Root"   : 2
}
numstore> get b;
{
    "Status" : "Ok",
    "Name"   : "b",
    "DType"  : "u32",
    "DSize"  : 4,
    "Nelems" : 0,
    "Bytes"  : 0,
    "Root"   : 3
}
numstore> exit;
```
