# styxrpc_live_open_until

[Русский](live_open_until.ru.md) · [Declaration](../include/styxrpc/live_client.h#L38) · [Implementation](../src/live_collection.c#L228) · [Guide](../LIVE_VALUES.md)

## Syntax

```c
#include <styxrpc/live_client.h>
StyxRpcIoOutcome styxrpc_live_open_until(struct StyxClient *session, const char *path,
    int writable, StyxRpcLive **out, StyxRpcVfsStop stop, void *state);
```

Link `StyxRpc::launch`.

## Parameters

| Name | Meaning |
| --- | --- |
| `session` | Mounted hosted client, borrowed for the entire binding lifetime. |
| `path` | Canonical relative NUL-terminated path of 1–1,024 bytes. It can include the published `.var` and `.prog` before `.var`. No leading slash, empty components or trailing slash. Scalar Int32 files are admitted inside `.var`. |
| `writable` | Access ceiling of 0 for read-only or 1 to allow granted writes. The peer's modes still apply. |
| `out` | Address receiving one owned binding on success. The previous value is untouched on failure. |
| `stop`, `state` | Optional synchronous stop callback and its borrowed argument. Return nonzero to stop. Use `NULL` for no callback. |

## Results

`STYXRPC_IO_OK` transfers one exact unopened fid and stores its checked type. Close it before destroying the session. `STYXRPC_IO_NOT_OPEN` covers invalid inputs, unavailable paths and unrecognized projections. A stopped walk or read may return `STYXRPC_IO_CANCELLED`. Transport, allocation or invalid transfer errors can also fail admission. Call on the session owner. The function retains neither `path` nor the stop callback.


## Example

```c
StyxRpcLive *values = NULL;
StyxRpcIoOutcome status = styxrpc_live_open_until(session, "live/.var/numbers",
    1, &values, NULL, NULL);
/* Retire values even if subsequent reads fail. */
styxrpc_live_close(values, 0);
```

[Full C example](../examples/disvm/LIVE_STAGE.md) · [Operation contract](../LIVE_VALUES.md#ownership-and-waiting)
