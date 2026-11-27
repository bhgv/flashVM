# styxrpc_cell_create_until

[Русский](cell_create_until.ru.md) · [Declaration](../include/styxrpc/named_cell.h#L66) · [Implementation](../src/named_channel_client.c#L240)

Create a cell with a cancellable setup, request and token read.

## Syntax

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_create_until(struct StyxClient *session,
    const void *frame, size_t size, void *token, StyxRpcVfsFile **out,
    StyxRpcNamedStop stop, void *state);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached Styx client and a provider that implements shared cells at `/.channels`. The tutorial is verified on Linux64 with DisVM.

## Parameters

| Name | Description |
| --- | --- |
| `session` | Borrowed attached client that reaches the issuing `/.channels` service. Keep it alive through return. |
| `frame` | Borrowed tagged value frame. Readable through return. |
| `size` | Frame length including its tag, 1 through `STYXRPC_NAMED_MAX` bytes. The tag determines further length constraints. |
| `token` | Writable storage for `STYXRPC_CELL_TOKEN` bytes (24). Filled on success. |
| `out` | Writable pointer slot. Receives an owned read/write fid only on success. |
| `stop` | Optional `StyxRpcNamedStop` callback, invoked on the calling thread. Return nonzero to request stop. NULL uses ordinary blocking waits. |
| `state` | Borrowed context for `stop`. It may be NULL if the callback accepts it. Keep it valid through return. |

## Return value

| Outcome | Condition |
| --- | --- |
| `STYXRPC_IO_OK` | Both outputs are published after the complete token arrives. The caller owns `*out`. |
| `STYXRPC_IO_NOT_OPEN` | Invalid arguments, unaccepted local value frame, or unavailable service. |
| `STYXRPC_IO_FAILED` | Definite admission failure, incomplete confirmed transfer or malformed token reply. |
| `STYXRPC_IO_CANCELLED` | Stop during the service walk, open, create request or token read. |
| `STYXRPC_IO_UNCERTAIN` | A command or reply has no reliable positive acknowledgement. |

## Behavior and lifetime

Use separate storage for `token` and `out`. Every failure preserves both outputs. A cancelled request may have created a cell. A temporary fid pins it until the server receives its cleanup. Close the returned fid only after intended users have acquired their own leases.

The callback should return promptly and avoid reentering the same session. Multiplexed Styx can flush a pending request by tag. Sequential sessions sample stop between exchanges. Temporary fid cleanup can continue after return.

## Example

Pass an attached session. The complete tutorial defines the deadline callback and closes the returned lease after sharing its token.

```c
unsigned char token[STYXRPC_CELL_TOKEN];
const unsigned char initial[] = {STYXRPC_NAMED_BOOLEAN, 1};
StyxRpcVfsFile *owner = NULL;
StyxRpcIoOutcome result = styxrpc_cell_create_until(session, initial,
    sizeof(initial), token, &owner, deadline_reached, &end);
```

After success, retain `owner` while sending the token. The example closes it explicitly after an alias is confirmed.

## See also

[Shared cells](named_cell.md) · [Value frames](cell_value.md) · [styxrpc_cell_create](cell_create.md) · [styxrpc_vfs_close](vfs_close.md) · [Runnable C and Limbo tutorial](../examples/shared-cell/README.md)
