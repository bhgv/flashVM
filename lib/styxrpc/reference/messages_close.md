# styxrpc_messages_close

[Русский](messages_close.ru.md) · [Header](../include/styxrpc/messages.h) · [Implementation](../src/message_client.c)

Include `styxrpc/messages.h` and link `StyxRpc::launch`.

```c
void styxrpc_messages_close(StyxRpcMessagePort *port);
```

Pass an owned client `port`. The function consumes it, discards queued
messages and releases its fid with normal Styx clunk semantics. It may
wait for the Styx deadline. Close the port before shutting down the
client. NULL is harmless. The caller cannot use the handle again.

Call on the owning thread outside callbacks using the session. Server
handles are borrowed from the dispatcher. The server consumes them on
clunk or disconnect, with one `closed` hook per opened port if present.

[Custom message setup](custom_messages.md) · [Worked socket example](../tests/message_socket.c)
