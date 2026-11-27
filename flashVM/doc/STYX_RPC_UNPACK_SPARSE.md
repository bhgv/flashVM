# vfs.unpackSparse

[Русский](STYX_RPC_UNPACK_SPARSE.ru.md) · [Live variables](STYX_RPC_LIVE_VARIABLES.md) · [Bridge value types](STYX_RPC_VALUE_TYPES.md) · [Implementation](../src/styxrpc/session_sparse.cpp#L133)

## Syntax

```actionscript
vfs.unpackSparse(tuple:Array):Array
```


## Parameter and result

Pass the native two-slot Array returned by [`packSparse`](STYX_RPC_PACK_SPARSE.md) or decoded from the corresponding typed tuple. The first slot is the logical length, 0–2,147,483,647. The second holds a dense Array of up to 2,048 two-slot pairs. Each index must be an integral `Number` in `[0, length)`. Indices must be strictly increasing. Use an empty Array of pairs for an all-hole or empty array.

Returns a new owner-local Array with exactly that length and one own data slot per pair. Missing indices have no own slot. Pair values are retained as local AS values.

## Errors and availability

Duplicate, unordered or out-of-range indices, a missing pair, a nil pair list, user accessors, watches or a wrong shape fail as invalid arguments. The partially filled Array is private and never becomes the result on failure. Allocation errors come from the VM. The helper runs synchronously without I/O on the movie or worker `vfs`.

## Example

```actionscript
// With tuple from the typed reply, index 5 can be absent even when length is 12.
var restored = vfs.unpackSparse(tuple);
var hasOwnFive = restored.hasOwnProperty("5");
```

[Paired example](../../lib/styxrpc/examples/disvm/LIVE_STAGE.md)
