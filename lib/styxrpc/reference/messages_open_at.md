# styxrpc_messages_open_at

[Русский](messages_open_at.ru.md) · [messages.h](../include/styxrpc/messages.h) · [messages_open](messages_open.md) · [UniBus](custom_messages.md)

`StyxRpc::launch`

```c
StyxRpcStatus styxrpc_messages_open_at(struct StyxClient *session, const char *name,
    const StyxRpcMessageSpec *spec, StyxRpcMessagePort **out);
```

On the attached `session`, open the named root read/write, copy `spec` and publish the owned client port in `out` on `STYXRPC_OK`. `STYXRPC_INVALID` leaves `out` unchanged.

The name is a single Styx root component. NULL selects `.unibus`. Call on the owner thread.
