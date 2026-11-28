# vfs.resultChannel

[Русский](STYX_RPC_RESULT_CHANNEL.ru.md) · [Guide](STYX_RPC_RESULT_CHANNELS.md) · [Implementation](../src/styxrpc/channel_result_endpoint.c#L247)

## Syntax

```actionscript
vfs.resultChannel(elementType:String, slots:Number):Object
```

## Parameters

| Parameter | Meaning |
| --- | --- |
| `elementType` | The compact type of one copied element, without the outer parentheses. Examples include `i`, `s`, `b`, `a[s]` and `l[(sb)]`. Use the same type as in the compiled `chan of T`. |
| `slots` | An exact integer from 0 to 8. A positive count applies separately to each direction. Zero selects synchronous rendezvous. |

## Return value and lifetime

Returns a bridge object with `trySend`, `tryReceive`, `active` and `close`. The first successful transfer from an exported method hands the publication to its result fid. Keep a script reference to use the local end. Ordinary value payloads, including nil strings, use the [typed mapping](STYX_RPC_VALUE_TYPES.md). Nested channel and reference types require other channel APIs.

Use the mounted FlashVM player or register the [C host constructor](STYX_RPC_RESULT_CHANNEL_FACTORY.md). Call on the context owner. An invalid type or slot count fails before any publication. Allocation or setup failure leaves no live object. The constructor does no Styx exchange. Returning a closed, previously transferred or mismatched endpoint causes a remote call error.

## Example

```actionscript
var owner = vfs.resultChannel("i", 1); // One item in each direction.
owner.trySend(21);                     // Save the owner, then return it from fn(): chan of int.
```

[Send](STYX_RPC_RESULT_SEND.md) · [Receive](STYX_RPC_RESULT_RECEIVE.md) · [Active](STYX_RPC_RESULT_ACTIVE.md) · [Close](STYX_RPC_RESULT_CLOSE.md)
