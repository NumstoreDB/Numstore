Wire Protocol
=============

From the Client's POV
```
connect(server)

write([ Q ][ "begin" ])
read()
    > [ S ]
    OR
    > [ E ][ CODE ]

write([ Q ][ "create a u32" ])
read()
    > [ S ]
    OR
    > [ E ][ CODE ]

write([ Q ][ "delete a" ])
read()
    > [ S ]
    OR
    > [ E ][ CODE ]

write([ Q ][ "remove a[0:10]" ])
read()
    > [ S ]
    OR
    > [ E ][ CODE ]

write([ Q ][ "get a" ])
OR
write([ Q ][ "read a[0:10]" ])
OR
write([ Q ][ "remove a[0:10] returning" ])
read()
    > [ D' ][ ... ]
      ... repeat [ D' ] ...
      > [ D ][ ... ]
    OR
    > [ S ]
    OR
    > [ E ][ CODE ]

write([ Q ][ "insert a[0:10]" ])
OR
write([ Q ][ "write a[0:10]" ])
read()
    > [ S ]
    OR
    > [ E ][ CODE ]
write([ D' ][ ... ])
    ... repeat [ D' ] ...
write([ D ][ ... ])
read()
    > [ S ]
    OR
    > [ E ][ CODE ]

write([ Q ][ "commit" ])
read()
    > [ S ]
    OR
    > [ E ][ CODE ]

write([ Q ][ "rollback" ])
read()
    > [ S ]
    OR
    > [ E ][ CODE ]

write([ C ])
shutdown()
```

From the Server's POV
```
accept(client)

read()
    > [ Q ][ "begin" ]
write([ S ])
OR
write([ E ][ CODE ])

read()
    > [ Q ][ "create a u32" ]
write([ S ])
OR
write([ E ][ CODE ])

read()
    > [ Q ][ "delete a" ]
write([ S ])
OR
write([ E ][ CODE ])

read()
    > [ Q ][ "remove a[0:10]" ]
write([ S ])
OR
write([ E ][ CODE ])

read()
    > [ Q ][ "get a" ]
    OR
    > [ Q ][ "read a[0:10]" ]
    OR
    > [ Q ][ "remove a[0:10] returning" ]
write([ D' ][ ... ])
    ... repeat [ D' ] ...
write([ D ][ ... ])
OR
write([ S ])
OR
write([ E ][ CODE ])

read()
    > [ Q ][ "insert a[0:10]" ]
    OR
    > [ Q ][ "write a[0:10]" ]
write([ S ])
OR
write([ E ][ CODE ])
read()
    > [ D' ][ ... ]
      ... repeat [ D' ] ...
    > [ D ][ ... ]
write([ S ])
OR
write([ E ][ CODE ])

read()
    > [ Q ][ "commit" ]
write([ S ])
OR
write([ E ][ CODE ])

read()
    > [ Q ][ "rollback" ]
write([ S ])
OR
write([ E ][ CODE ])

read()
    > [ C ]
close(client)
```
