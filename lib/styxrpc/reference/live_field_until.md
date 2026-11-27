# styxrpc_live_field_until

[Русский](live_field_until.ru.md) · [Declaration](../include/styxrpc/live_client.h#L51) · [Implementation](../src/live_collection.c#L245) · [Guide](../LIVE_VALUES.md)

## Syntax

```c
#include <styxrpc/live_client.h>
StyxRpcIoOutcome styxrpc_live_field_until(const StyxRpcLive *live, const char *name,
    StyxRpcLive **out, StyxRpcVfsStop stop, void *state);
```

Link `StyxRpc::launch`.

## Parameters and result

Borrow a record `live`, a nonempty single field `name` without slash or initial dot and optional `stop(state)` as for [open](live_open_until.md). `out` is writable pointer storage. Parent and child paths must fit the live-path limit. `STYXRPC_IO_OK` installs a new independently owned field binding. It inherits the parent's write ceiling and the peer's field permissions. Closing the parent has no effect on that ownership. Keep the session open for both bindings.

`STYXRPC_IO_NOT_OPEN` covers a bad name, a nonrecord or stale parent and unavailable or foreign field. Cancellation or failed I/O can interrupt the synchronous walk. On failure `out` is unchanged.


## Example

```c
StyxRpcLive *name = NULL;
if (styxrpc_live_field_until(details, "name", &name, NULL, NULL) == STYXRPC_IO_OK) {
    /* The child can now be used without retaining details. */
    styxrpc_live_close(name, 0);
}
```

[Full C example](../examples/disvm/LIVE_STAGE.md) · [Operation contract](../LIVE_VALUES.md#ownership-and-waiting)
