# styxrpc_messages_publish

[Русский](messages_publish.ru.md) · [messages.h](../include/styxrpc/messages.h) · [messages_enable](messages_enable.md) · [UniBus](custom_messages.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_messages_publish(StyxRpcServer *server, const char *name,
    const StyxRpcMessageSpec *spec);
```

The owner thread can publish before the first server step or during service,
outside exchange callbacks. The server copies `name` and `spec`. A name or Qid
collision, exhaustion or invalid registration returns `STYXRPC_INVALID`.

The name is a single Styx root component. NULL selects `.unibus`. Call on the owner thread.

Dynamic trees reserve published names and the exchange Qid range
(descending from UINT64_MAX − 1). The module-slot tree reserves the exact name
against ordinary files and named channels until
[withdrawal](messages_withdraw.md). Keep callback state alive through
withdrawal or server destruction.
