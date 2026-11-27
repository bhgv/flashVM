# styxrpc_messages_enable

[Русский](messages_enable.ru.md) · [Header](../include/styxrpc/messages.h#L87) · [Implementation](../src/serve_messages.inc)

Include `styxrpc/messages.h` and link `StyxRpc::serve`.

```c
StyxRpcStatus styxrpc_messages_enable(StyxRpcServer *server, const StyxRpcMessageSpec *spec);
```

Call with an existing hosted `server` and a registration `spec` before the first
dispatch step. The SDK copies the descriptors and names. Keep receiver and
hook contexts alive until [withdrawal](messages_withdraw.md) or server
destruction. A NULL hook is allowed.
`opened` exposes a borrowed port that can queue messages. `closed` runs just
before its disposal, including on disconnect. Callbacks run on the dispatcher
and must return before another request can be served.

Returns `STYXRPC_OK` and publishes `.messages`, or `STYXRPC_INVALID` for an
invalid spec, allocation failure, repeated or late enable, or a tree conflict.
A tree using the service reserves the root name `.messages` and the
[exchange Qid range](../include/styxrpc/exchange_qid.h). All attached peers that
can open the file read/write can invoke its registered handlers.

[Custom message setup](custom_messages.md) · [Worked socket example](../tests/message_socket.c)
