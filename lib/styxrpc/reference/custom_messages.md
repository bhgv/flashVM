# Custom UniBus messages

[Русский](custom_messages.ru.md) · [SDK build](../README.md#build)

Use a message port when both applications need to send typed events or binary
data at their own pace. For example, an instrument may publish samples and
accept calibration updates over the same mounted Styx session that exposes its
files and named methods. A message is an opaque payload registered by a
case-sensitive schema name and nonzero version. The application owns its schema
and payload codec.

The hosted server can expose `.messages` at the root alongside its existing
tree. Opening the file read/write creates a bidirectional stream with two
independent byte offsets and a registry for each direction. Closing its fid
releases only that stream. The client uses an existing `StyxClient`, including
one on a bidirectional link.

## Set up one port

1. Install the hosted SDK with `STYXRPC_HOSTED=ON`. Include
   `styxrpc/messages.h`. Link `StyxRpc::serve` on the server and
   `StyxRpc::launch` on the client.
2. On each side, list the types in a `StyxRpcMessageSpec`. Match the schema's
   name and version. Choose a locally unique nonzero `send_tag` for each type
   you send. Set a `receive` callback for each type you accept. For example,
   a server sending `sensor.sample` version 1 may use tag 6 and a NULL
   handler, while a receive-only client registers it with tag 0 and a handler.
3. Call [styxrpc_messages_enable](messages_enable.md) before the server's first
   dispatch step. The server can send from its `opened` hook, receive
   callbacks, or between dispatch steps using the port obtained in the hook.
4. Call [styxrpc_messages_open](messages_open.md) on the mounted client. Call
   [styxrpc_message_send](message_send.md) to queue a copy, and
   [styxrpc_messages_pump](messages_pump.md) on the client owner to move bytes
   both ways. Process incoming events in the registered handlers. Schedule
   another pump when the application next polls the port or has queued output.
5. Call [styxrpc_messages_close](messages_close.md) on the client. The server's
   `closed` hook retires the borrowed port and also runs if the server closes
   its fids after a disconnect.

Each receiver borrows the payload only through its callback and returns 1 to
accept it. A receiver may call `styxrpc_message_send` on that port. The
application provides scheduling and keeps the port on the owning I/O thread.
Set limits or validate the syntax of application payloads in your handlers.

## Wire and failure behavior

The first record in each direction is the standard UniBus tag-zero binding
declaration. It lists the sender's nonzero tags and their schema names and
versions. Each subsequent record uses one of those tags. The receiving side
checks the completed S-record and looks up its handler by the declared schema.
The registry is fixed per open. Unknown schema declarations are tolerated.
Data with an unknown tag, an unsupported schema, a failed checksum or a
handler that rejects it retires the port. This also applies to data before a
declaration and to a repeated declaration. Other fids on the Styx session
can continue to operate.

On the raw file, an empty read means the output queue is temporarily empty.
A regular read or write must start at the next position for its direction.
The client pump never replays an uncertain write. A write can contain several
records, so a failure may follow the processing of earlier records. An
application that needs confirmations can register a reply schema and carry
request IDs in its payload.

Limits are 32 registrations per side, 16,384 payload bytes per record, and
eight ports per hosted server. The example in
[`tests/message_socket.c`](../tests/message_socket.c) mounts one real socket,
exchanges a maximum-size binary sample in fragmented Styx transfers, and
calls `increment` on the same client. Run it from the SDK source directory.

```sh
cmake -S . -B build -DSTYXRPC_HOSTED=ON -DCMAKE_PREFIX_PATH=/path/to/prefix
cmake --build build
ctest --test-dir build -R styxrpc-custom-messages --output-on-failure
```
