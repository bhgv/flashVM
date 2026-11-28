# endpoint.active

[Русский](STYX_RPC_RESULT_ACTIVE.ru.md) · [Guide](STYX_RPC_RESULT_CHANNELS.md) · [Implementation](../src/styxrpc/channel_result_endpoint.c#L177)

## Syntax

```actionscript
endpoint.active():Boolean
```

Takes no arguments. Returns `true` while the endpoint is ready for publication or its issued resource is live. Returns `false` after local close, a rejected publication that consumed the resource, or retirement of the remote lease. Takes a native snapshot under the mailbox mutex. The result reserves no send or receive and can change before the next operation.

```actionscript
if (owner.active()) {
    last = owner.tryReceive();
}
```

[Close](STYX_RPC_RESULT_CLOSE.md) · [C active check](../../lib/styxrpc/reference/channel_mailbox_active.md)
