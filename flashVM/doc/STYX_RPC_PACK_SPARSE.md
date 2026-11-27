# vfs.packSparse

[Русский](STYX_RPC_PACK_SPARSE.ru.md) · [Live variables](STYX_RPC_LIVE_VARIABLES.md) · [Bridge value types](STYX_RPC_VALUE_TYPES.md) · [Implementation](../src/styxrpc/session_sparse.cpp#L34)

## Syntax

```actionscript
vfs.packSparse(array:Array):Array
```


## Parameters and result

Pass a native data Array owned by the current context. Length may be 0–2,147,483,647 with at most 2,048 own indexed slots. The method accepts the standard Array length accessor. User accessors and watches on the input are refused. It reads the raw own indexed slots and ignores named properties and inherited values.

Returns `[length, [[index, value], …]]` with strictly increasing indices. The container and each pair are new Arrays. Slot values are the original local values until the RPC encoder copies them using the declared `T`. The canonical typed tuple is `(ia[(iT)])`. Use `(int, array of (int, T))` in Limbo. Pack can preserve an occupied slot whose value is `undefined` in memory. Sending it requires a value admitted by the declared `T`.

## Errors and availability

Malformed arguments and data-array shape violations are argument errors. Allocation failure is reported by the VM. The movie or `asinc` worker can call this helper without remote I/O once its `vfs` surface is present. The typed call retains its depth and frame limits.

## Example

```actionscript
// Retain two trailing holes as well as the hole before the value.
var samples = new Array(6);
samples[3] = "ready";
var encoded = vfs.packSparse(samples); // [6, [[3, "ready"]]]
```

[Restore a sparse array](STYX_RPC_UNPACK_SPARSE.md) · [Run through Limbo](../../lib/styxrpc/examples/disvm/LIVE_STAGE.md)
