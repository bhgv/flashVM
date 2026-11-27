# styxrpc_cell_read_until

[Русский](cell_read_until.ru.md) · [Declaration](../include/styxrpc/named_cell.h#L82) · [Implementation](../src/named_channel_client.c#L346)

Read a complete frame by token through a temporary fid.

## Syntax

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_read_until(struct StyxClient *session,
    const void *token, unsigned char **bytes, size_t *size,
    StyxRpcNamedStop stop, void *state);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached Styx client and a provider that implements shared cells at `/.channels`. The tutorial is verified on Linux64 with DisVM.

## Parameters

| Name | Description |
| --- | --- |
| `session` | Borrowed attached client that reaches the issuing `/.channels` service. Keep it alive through return. |
| `token` | Borrowed readable token of `STYXRPC_CELL_TOKEN` bytes (24). Keep a lease or transfer pin alive through return. |
| `bytes` | Writable output pointer. Receives a buffer allocated by `malloc` on success. Release it with `free`. |
| `size` | Writable output for the exact frame length in bytes, including its tag. |
| `stop` | Optional `StyxRpcNamedStop` callback, invoked on the calling thread. Return nonzero to request stop. NULL uses ordinary blocking waits. |
| `state` | Borrowed context for `stop`. It may be NULL if the callback accepts it. Keep it valid through return. |

## Return value

| Outcome | Condition |
| --- | --- |
| `STYXRPC_IO_OK` | Transfers the complete owned frame and its byte length. |
| `STYXRPC_IO_NOT_OPEN` | Invalid pointers or unavailable service. |
| `STYXRPC_IO_FAILED` | Definite setup problem or an invalid, incomplete or empty confirmed reply. |
| `STYXRPC_IO_CANCELLED` | Stop during attachment, submission of the read command or receipt of its reply. |
| `STYXRPC_IO_UNCERTAIN` | An attempted command or read lacks a reliable positive acknowledgement. No output is published. |

## Behavior and lifetime

The SDK attaches and retires its own fid. Keep the original lease live throughout. Failure preserves both output slots. On success, the caller can decode the frame independently of later edits. Concurrent reads use separate output storage.

The callback should return promptly and avoid reentering the same session. Multiplexed Styx can flush a pending request by tag. Sequential sessions sample stop between exchanges. Temporary fid cleanup can continue after return.

## Example

The original lease must remain live. This reads by its token while polling the tutorial deadline.

```c
unsigned char *frame = NULL;
size_t size = 0;
if (styxrpc_cell_read_until(session, token, &frame, &size,
        deadline_reached, &end) == STYXRPC_IO_OK)
{
    free(frame);
}
```

Include `<stdlib.h>` for `free`. The complete example checks the two bytes of a Boolean before freeing the frame.

## See also

[Shared cells](named_cell.md) · [Value frames](cell_value.md) · [styxrpc_cell_get](cell_get.md) · [styxrpc_vfs_close](vfs_close.md) · [Runnable C and Limbo tutorial](../examples/shared-cell/README.md)
