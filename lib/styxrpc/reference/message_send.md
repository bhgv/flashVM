# styxrpc_message_send

[Русский](message_send.ru.md) · [Header](../include/styxrpc/messages.h) · [Implementation](../src/message_port.c)

Include `styxrpc/messages.h` and link `StyxRpc::rpc`.

```c
StyxRpcStatus styxrpc_message_send(StyxRpcMessagePort *port, const char *name, uint32_t version,
    const void *payload, size_t size);
```

Specify a live `port`, the exact registered `name` and `version` with a
nonzero send tag, and `size` bytes at `payload`. The size may be 0 to 16,384.
NULL is allowed for an empty payload. The port copies the bytes on
`STYXRPC_OK`. This acknowledges queuing only. `STYXRPC_MORE` means the
queue is full and the message was skipped. `STYXRPC_INVALID` means a bad
argument, missing send registration or a failed port.

Call from the port owner, including a receive or opened callback. It
does no network I/O. The client moves bytes with
[styxrpc_messages_pump](messages_pump.md). The server moves them in response
to reads of that fid. Complete delivery and application acceptance require
a peer-defined reply if the sender needs confirmation.

[Custom message setup](custom_messages.md) · [Worked socket example](../tests/message_socket.c)
