# styxrpc_live_write_until

[Русский](live_write_until.ru.md) · [Declaration](../include/styxrpc/live_client.h#L70) · [Implementation](../src/live_collection.c#L437) · [Guide](../LIVE_VALUES.md)

## Syntax

```c
#include <styxrpc/live_client.h>
StyxRpcIoOutcome styxrpc_live_write_until(const StyxRpcLive *live, int64_t index,
    const StyxRpcLiveValue *value, StyxRpcVfsStop stop, void *state);
```

Link `StyxRpc::launch`.

## Parameters

Borrow a writable `live` binding and the requested `value`. Index rules match [read](live_read_until.md). Set `value->integer` for numeric writes. Int32 accepts the signed 32-bit interval. For text set `nil` to 0 and supply valid counted UTF-8 in `text` with `size` at most 1,024 bytes, or set `nil` to 1 with zero size. No terminator is required. The callback `stop` and `state` follow [open](live_open_until.md).

## Results and effects

`STYXRPC_IO_OK` confirms one complete write at offset zero. Admission rejects a read-only or stale binding and an invalid index or value. `STYXRPC_IO_CANCELLED` can stop the setup. `STYXRPC_IO_UNCERTAIN` indicates the peer might have applied the value. Reconcile by reading the still-current binding or checking application state. The SDK sends no remainder or retry on a failed or uncertain write. Use the calling session's serialized owner.


## Example

```c
StyxRpcLiveValue replacement = {0};
replacement.integer = 42;
StyxRpcIoOutcome status = styxrpc_live_write_until(values, 0, &replacement, NULL, NULL);
/* A retry is a separate application decision after reconciliation. */
```

[Full C example](../examples/disvm/LIVE_STAGE.md) · [Operation contract](../LIVE_VALUES.md#ownership-and-waiting)
