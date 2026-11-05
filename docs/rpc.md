# The numstore RPC

Four verbs mirroring the public numstore API: PUSH, PULL, EXECUTE, GET.
Queries are plain strings in the numstore query language.

`libs/nsserver` holds the service and the proto, `apps/numstore` serves it, and
`apps/nsclient` is a demo client. Nothing is wired to numstore yet - every
server behaviour lives in `libs/nsserver/src/hooks.rs`, which is the only file
that changes when it is.

## Errors

Errors are an in-band message, not a gRPC status:

```proto
message Error { int32 code = 1; string message = 2; }
```

`tonic::Status` is reserved for transport and framework failures. After the
server sends an `Error` it ends the stream.

| Code | Meaning |
|---|---|
| 1 | verb not allowed on this RPC |
| 2 | more bytes pushed than the query asked for |
| 3 | malformed query |

## Verb validation

The server checks the leading verb of each query against the RPC it arrived on.
Beyond the verb, the query is an opaque string.

| RPC | Allowed verbs |
|---|---|
| Push | `insert`, `write` |
| Pull | `read`, `remove` |
| Execute | `create`, `delete`, `remove` |
| Get | any |

A wrong verb is `Error` code 1, not a `Status`.

## Push - client-streaming data, per-chunk acks

```text
Client                                   Server
  |  PUSH "insert foo 10 100"              |
  |--------------------------------------->|
  |                  Ok                    |
  |<---------------------------------------|
  |  [data chunk]                          |
  |--------------------------------------->|
  |                  Ok                    |
  |<---------------------------------------|
  |  [data chunk]                          |
  |--------------------------------------->|
  |             Ok | Error                 |
  |<---------------------------------------|
```

The first request message is the query; every later message is a data chunk.
The server acks each message with `Ok`.

The last token of the query is the total byte count the server expects (100
above). The hook tracks what is left. Sending fewer bytes is fine - the server
keeps waiting. Sending more is `Error` code 2.

## Pull - server-streaming, lazy

```text
Client                                   Server
  |  PULL "read foo[0:10]"                 |
  |--------------------------------------->|
  |            [data chunk]                |
  |<---------------------------------------|
  |            [data chunk]                |
  |<---------------------------------------|
  |        end of stream | Error           |
  |<---------------------------------------|
```

There is deliberately **no** Ready/Ok handshake per chunk. HTTP/2 flow control
already provides backpressure: the client stops reading, the window fills, and
the server stops being polled. The client cancels by dropping the stream.

The response stream is built with `futures::stream::unfold`, so the reader hook
is called once per chunk the transport can accept - never up front, and never
through an unbounded channel.

The reader hook is shaped "read up to N bytes, return the bytes, empty means
end", because that is how the real internal function works. It is synchronous
in spirit; when the real reader lands, the call needs `spawn_blocking`.

Reader cleanup is in `Drop`, since cancellation is a drop.

## Execute - unary

```text
Client                                   Server
  |  EXECUTE "create foo ..."              |
  |--------------------------------------->|
  |             Ok | Error                 |
  |<---------------------------------------|
```

`remove` is allowed on both Pull (which returns the removed data) and Execute
(which discards it).

## Get - unary

```text
Client                                   Server
  |  GET "get foo"                         |
  |--------------------------------------->|
  |         Var(bytes) | Error             |
  |<---------------------------------------|
```

A variable comes back as raw `bytes`, so the format can change without a proto
change.

## Chunk size

One constant, 64 KiB (`hooks::CHUNK`), well under tonic's 4 MiB default
message limit.

## Running it

```sh
cargo run -p numstore-app --bin numstore-server   # one terminal
cargo run -p nsclient                             # another
```

The client runs a push, a pull, an execute, a get, and the two error paths.

`build.rs` sets `PROTOC` from `protoc-bin-vendored`, so no system `protoc` is
needed.
