# styxrpc_cell_write_until

[Русский](cell_write_until.ru.md) · [Declaration](../include/styxrpc/named_cell.h#L90) · [Implementation](../src/named_channel_client.c#L367)

Replace a cell frame by token through a cancellable temporary fid.

## Syntax

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_write_until(struct StyxClient *session,
    const void *token, const void *frame, size_t size, StyxRpcNamedStop stop, void *state);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached Styx client and a provider that implements shared cells at `/.channels`. The tutorial is verified on Linux64 with DisVM.

## Parameters

| Name | Description |
| --- | --- |
| `session` | Borrowed attached client that reaches the issuing `/.channels` service. Keep it alive through return. |
| `token` | Borrowed readable token of `STYXRPC_CELL_TOKEN` bytes (24). Keep a lease or transfer pin alive through return. |
| `frame` | Borrowed tagged value frame. Readable through return. |
| `size` | Frame length including its tag, 1 through `STYXRPC_NAMED_MAX` bytes. The tag determines further length constraints. |
| `stop` | Optional `StyxRpcNamedStop` callback, invoked on the calling thread. Return nonzero to request stop. NULL uses ordinary blocking waits. |
| `state` | Borrowed context for `stop`. It may be NULL if the callback accepts it. Keep it valid through return. |

## Return value

| Outcome | Condition |
| --- | --- |
| `STYXRPC_IO_OK` | Confirms the complete replacement. An original positive reply can win the race with stop. |
| `STYXRPC_IO_NOT_OPEN` | Invalid input, rejected local value frame or unavailable service. |
| `STYXRPC_IO_FAILED` | Definite setup failure or short confirmed transfer. |
| `STYXRPC_IO_CANCELLED` | Stop before a possibly effective value write. A header or a partial staging prefix can already have been accepted. |
| `STYXRPC_IO_UNCERTAIN` | An attempted command or value fragment lacks positive acknowledgement. If the full body reached the issuer, the replacement may already be visible. |

## Behavior and lifetime

The SDK uses a separate operation fid and sends each confirmed offset once. An incomplete frame never replaces the cell. Clunk preserves a replacement already accepted. A recovery read sees the current frame and can race with another holder's write. Application-level read/modify/write requires coordination between writers.

The callback should return promptly and avoid reentering the same session. Multiplexed Styx can flush a pending request by tag. Sequential sessions sample stop between exchanges. Temporary fid cleanup can continue after return.

## Example

Hold the issuing lease while assigning. The tutorial defines the deadline callback.

```c
const unsigned char enabled[] = {STYXRPC_NAMED_BOOLEAN, 1};
StyxRpcIoOutcome result = styxrpc_cell_write_until(session, token,
    enabled, sizeof(enabled), deadline_reached, &end);
```

Keep a separate lease through return. After `STYXRPC_IO_UNCERTAIN`, use `styxrpc_cell_read_until` with a new live stop state if the application needs the visible value.

## See also

[Shared cells](named_cell.md) · [Value frames](cell_value.md) · [styxrpc_cell_set](cell_set.md) · [styxrpc_vfs_close](vfs_close.md) · [Runnable C and Limbo tutorial](../examples/shared-cell/README.md)
