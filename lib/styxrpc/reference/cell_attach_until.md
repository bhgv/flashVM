# styxrpc_cell_attach_until

[Русский](cell_attach_until.ru.md) · [Declaration](../include/styxrpc/named_cell.h#L74) · [Implementation](../src/named_channel_client.c#L283)

Acquire an independent cell fid while allowing cancellation.

## Syntax

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_attach_until(struct StyxClient *session,
    const void *token, StyxRpcVfsFile **out, StyxRpcNamedStop stop, void *state);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached Styx client and a provider that implements shared cells at `/.channels`. The tutorial is verified on Linux64 with DisVM.

## Parameters

| Name | Description |
| --- | --- |
| `session` | Borrowed attached client that reaches the issuing `/.channels` service. Keep it alive through return. |
| `token` | Borrowed readable token of `STYXRPC_CELL_TOKEN` bytes (24). Keep a lease or transfer pin alive through return. |
| `out` | Writable pointer slot. Receives an owned read/write fid only on success. |
| `stop` | Optional `StyxRpcNamedStop` callback, invoked on the calling thread. Return nonzero to request stop. NULL uses ordinary blocking waits. |
| `state` | Borrowed context for `stop`. It may be NULL if the callback accepts it. Keep it valid through return. |

## Return value

| Outcome | Condition |
| --- | --- |
| `STYXRPC_IO_OK` | Publishes one owned fid after full acknowledgement of the token. |
| `STYXRPC_IO_NOT_OPEN` | Invalid arguments or unavailable service. |
| `STYXRPC_IO_FAILED` | Definite local failure or short confirmed command. |
| `STYXRPC_IO_CANCELLED` | Stop during service setup or token submission. |
| `STYXRPC_IO_UNCERTAIN` | The token write lacks positive acknowledgement. This can include a Styx `Rerror` for a foreign or revoked token. |

## Behavior and lifetime

Finish attachment before the next named-channel request can release the transfer pin. Failure preserves `out` and retires the temporary fid. On success, close the owned fid with `styxrpc_vfs_close`.

The callback should return promptly and avoid reentering the same session. Multiplexed Styx can flush a pending request by tag. Sequential sessions sample stop between exchanges. Temporary fid cleanup can continue after return.

## Example

Pass the received live token and an attached session. The tutorial defines the deadline callback.

```c
StyxRpcVfsFile *alias = NULL;
StyxRpcIoOutcome result = styxrpc_cell_attach_until(session, token, &alias,
    deadline_reached, &end);
```

The token comes from the same registry. After success use the new lease or keep it open while reading by token.

## See also

[Shared cells](named_cell.md) · [Value frames](cell_value.md) · [styxrpc_cell_attach](cell_attach.md) · [styxrpc_vfs_close](vfs_close.md) · [Runnable C and Limbo tutorial](../examples/shared-cell/README.md)
