# styxrpc_live_length_until

[Русский](live_length_until.ru.md) · [Declaration](../include/styxrpc/live_client.h#L57) · [Implementation](../src/live_collection.c#L269) · [Guide](../LIVE_VALUES.md)

## Syntax

```c
#include <styxrpc/live_client.h>
StyxRpcIoOutcome styxrpc_live_length_until(const StyxRpcLive *live, uint32_t *length,
    StyxRpcVfsStop stop, void *state);
```

Link `StyxRpc::launch`.

## Parameters and result

Pass a borrowed collection or record `live` and writable `length` storage for the exact count. The optional `stop(state)` callback follows [open](live_open_until.md). `STYXRPC_IO_OK` reads the provider's `.len` in the range 0–4,294,967,295. It reports element count for arrays and vectors, named field count for a record and dense length for a hybrid table. A later access can see a newer length.

A scalar, a missing or replaced projection and a missing length field return an unavailable outcome. Failure can also indicate invalid metadata, transport failure or cancellation. `length` is unchanged on failure. The synchronous operation closes its own metadata fid.


## Example

```c
uint32_t count;
if (styxrpc_live_length_until(values, &count, NULL, NULL) == STYXRPC_IO_OK && count > 0) {
    /* Position 0 existed when .len was read. */
}
```

[Full C example](../examples/disvm/LIVE_STAGE.md) · [Operation contract](../LIVE_VALUES.md#ownership-and-waiting)
