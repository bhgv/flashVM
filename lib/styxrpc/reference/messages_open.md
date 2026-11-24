# styxrpc_messages_open

[Русский](messages_open.ru.md) · [Header](../include/styxrpc/messages.h) · [Implementation](../src/message_client.c)

Include `styxrpc/messages.h` and link `StyxRpc::launch`.

```c
StyxRpcStatus styxrpc_messages_open(struct StyxClient *session, const StyxRpcMessageSpec *spec,
    StyxRpcMessagePort **out);
```

`session` is an attached client whose root publishes `.messages`. `spec`
supplies one to 32 local type registrations. The SDK copies their names and
descriptors. Its contexts must remain valid through close. Client lifecycle
uses `types` and `count` only. Each descriptor needs a nonzero version, a
name of 1 to 63 printable non-space ASCII bytes and a send tag, a receiver
or both. Repeated name/version pairs and repeated nonzero tags are invalid.

The operation opens a read/write fid synchronously with ordinary Styx
deadlines. `STYXRPC_OK` fills `out` with a client-owned port. On failure
`STYXRPC_INVALID` leaves `out` untouched. The registry is fixed until close.
Keep `session` and its owner alive while using the port. Drive the binding
exchange and callbacks with [styxrpc_messages_pump](messages_pump.md).

[Custom message setup](custom_messages.md) · [Worked socket example](../tests/message_socket.c)
