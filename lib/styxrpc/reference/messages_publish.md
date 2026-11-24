# styxrpc_messages_publish

[Русский](messages_publish.ru.md) · [messages.h](../include/styxrpc/messages.h) · [messages_enable](messages_enable.md) · [UniBus](custom_messages.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_messages_publish(StyxRpcServer *server, const char *name,
    const StyxRpcMessageSpec *spec);
```

Before its first step, `server` copies `name` and `spec`. A name or Qid collision, exhaustion or invalid registration returns `STYXRPC_INVALID`.

The name is a single Styx root component. NULL selects `.unibus`. Call on the owner thread.

Dynamic trees must reserve the published names, their Qids (descending from
UINT64_MAX − 1) and their directory slots through server destruction.
