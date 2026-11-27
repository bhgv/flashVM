# binding.set

[Русский](STYX_RPC_LIVE_SET.ru.md) · [Live variables](STYX_RPC_LIVE_VARIABLES.md) · [Bridge value types](STYX_RPC_VALUE_TYPES.md) · [Implementation](../src/styxrpc/session_live_values.c#L152)

## Syntax

```actionscript
binding.set([index, ]value):Boolean
```


## Parameters

Choose the position as in [`get`](STYX_RPC_LIVE_GET.md). Pass a whole signed Int32 `Number` for an Int32 cell or array element. For `big`, pass a safe integral `Number` or an exact signed decimal `String`. For a string cell, pass `null` for nil or a UTF-8 `String` of at most 1,024 bytes. An empty string is a value, distinct from nil. Embedded zero is preserved.

## Result and effects

On acknowledged replacement returns `true`. The original owner's scalar or selected cell receives one complete write. An Int32 array is fixed. A `big` vector accepts existing positions, including relative positions. A hybrid table's numeric space follows the owner's key admission rules. A readable binding can fail a write if the owner grants read access only.

## Errors and waiting

Wrong argument count, type or range raises an argument error. Stale identity, refused selection, failed write or cancellation raises an exception. The call is synchronous and observes stop in `asinc`. A write with an uncertain result may have taken effect. Read the same still-current binding or reconcile application state before trying again. The implementation never replays the write.

## Example

```actionscript
// Commit the exact signed maximum to a scalar big cell.
var magnitude = vfs.liveVariable("/live/.var/details/magnitude", true);
var acknowledged = magnitude.set("9223372036854775807");
var closed = magnitude.close();
```
