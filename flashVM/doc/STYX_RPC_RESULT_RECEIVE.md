# endpoint.tryReceive

[Русский](STYX_RPC_RESULT_RECEIVE.ru.md) · [Guide](STYX_RPC_RESULT_CHANNELS.md) · [Implementation](../src/styxrpc/channel_result_endpoint.c#L115)

## Syntax

```actionscript
endpoint.tryReceive():Object
```

Takes no arguments. Returns a copied typed value from the peer or `undefined` when the incoming FIFO is empty or no peer sender is waiting in rendezvous mode. A nil string returns `null`. A closed endpoint raises an exception. After consuming a message the method decodes it on the AS owner, where allocation or conversion can still fail. No value is automatically replayed. Call between movie frames or owner steps without waiting for network traffic.

```actionscript
var message = owner.tryReceive();
if (message !== undefined) {
    latest = message;              // A nil string is a received value.
}
```

[Send](STYX_RPC_RESULT_SEND.md) · [Type mapping](STYX_RPC_VALUE_TYPES.md)
