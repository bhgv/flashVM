# styxrpc_live_read_until

[Русский](live_read_until.ru.md) · [Declaration](../include/styxrpc/live_client.h#L62) · [Implementation](../src/live_collection.c#L380) · [Guide](../LIVE_VALUES.md)

## Syntax

```c
#include <styxrpc/live_client.h>
StyxRpcIoOutcome styxrpc_live_read_until(const StyxRpcLive *live, int64_t index,
    StyxRpcLiveValue *out, StyxRpcVfsStop stop, void *state);
```

Link `StyxRpc::launch`.

## Parameters

Borrow `live` and pass `index` as zero for a scalar or string. The fixed Int32 array takes a nonnegative index below its current length. A `big` vector accepts a signed position, including `-1` for its final element. A hybrid table takes its exact signed numeric key and resolves a negative index relative to its dense length. `out` points to the caller's result storage. Pass optional `stop(state)` as for [open](live_open_until.md).

## Results and lifetime

`STYXRPC_IO_OK` publishes a native signed 64-bit `out->integer` or, for text, `out->nil` and `out->size` bytes of UTF-8 in `out->text`. Text has no guaranteed terminator. A zero size with `nil == 0` is the empty string. Embedded zero is valid. The result is a copy.

A stopped, failed or refused operation leaves `*out` unchanged. An invalid index, nonvalue node, stale publication or missing cell can be refused. A malformed record fails instead of publishing a prefix. The temporary value and selector fids are closed even after refusal. The host owns and serializes the connection.


## Example

```c
StyxRpcLiveValue sample;
if (styxrpc_live_read_until(values, 0, &sample, NULL, NULL) == STYXRPC_IO_OK) {
    /* sample.integer now holds the selected signed value. */
}
```

[Full C example](../examples/disvm/LIVE_STAGE.md) · [Operation contract](../LIVE_VALUES.md#ownership-and-waiting)
