# styxrpc_live_kind

[Русский](live_kind.ru.md) · [Declaration](../include/styxrpc/live_client.h#L44) · [Implementation](../src/live_collection.c#L238) · [Guide](../LIVE_VALUES.md)

## Syntax

```c
#include <styxrpc/live_client.h>
StyxRpcLiveKind styxrpc_live_kind(const StyxRpcLive *live);
```

Link `StyxRpc::launch`.

## Parameter and return

Borrow a valid, still-owned `live` binding. Returns the local immutable projection kind. The call reads no remote data. It has no error return and requires a non-null pointer. The kind remains readable after removal of the remote publication while the binding is owned. Before doing I/O, the other operations still verify the published identity.

`STYXRPC_LIVE_INT` and `STYXRPC_LIVE_BIG` denote scalars. `STYXRPC_LIVE_INTS` is a fixed signed Int32 array. `STYXRPC_LIVE_BIGS` is a `big` vector or the numeric space of a hybrid table. `STYXRPC_LIVE_TEXT` is a counted string or nil. `STYXRPC_LIVE_ROW` is a directory of named fields.


## Example

```c
if (styxrpc_live_kind(binding) == STYXRPC_LIVE_ROW) {
    /* Select a named field before reading a scalar value. */
}
```

[Full C example](../examples/disvm/LIVE_STAGE.md) · [Operation contract](../LIVE_VALUES.md#ownership-and-waiting)
