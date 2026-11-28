# endpoint.trySend

[Русский](STYX_RPC_RESULT_SEND.ru.md) · [Guide](STYX_RPC_RESULT_CHANNELS.md) · [Implementation](../src/styxrpc/channel_result_endpoint.c#L68)

## Syntax

```actionscript
endpoint.trySend(value):Boolean
```

Pass one value of the [endpoint's element type](STYX_RPC_RESULT_CHANNEL.md). Encoding copies the value by the compiled schema on the context owner. Returns `true` after buffering the value or matching a waiting peer receive. Returns `false` when the outgoing buffer is full or when a rendezvous receiver is absent. The value stays with the caller in the latter cases. A closed endpoint or an incompatible value raises an exception. The operation makes no network wait. If a peer retires the lease after a successful buffered send, the unread value can be discarded.

```actionscript
var sent = owner.trySend("ready"); // For an endpoint created with "s".
if (!sent) {
    pendingText = "ready";          // Keep this value for the next owner tick.
}
```

[Receive](STYX_RPC_RESULT_RECEIVE.md) · [Example](../../lib/styxrpc/examples/disvm/CHANNEL_RESULTS.md)
