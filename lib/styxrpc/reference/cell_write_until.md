# styxrpc_cell_write_until

[Русский](cell_write_until.ru.md) · [Shared cells](named_cell.md) · [Declaration](../include/styxrpc/named_cell.h#L90) · [Implementation](../src/named_channel_client.c#L367)

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_write_until(
    struct StyxClient *session, const void *token, const void *frame,
    size_t size, StyxRpcNamedStop stop, void *state);
```

Replace the complete [value frame](cell_value.md) on the issuing `session`.
Borrow a `STYXRPC_CELL_TOKEN`-byte `token` and `size` bytes at `frame`.
Keep the original lease or a transfer pin live. The SDK uses a temporary
fid and retires it on every outcome. Other holders keep their access.

`STYXRPC_IO_OK` confirms the whole replacement. A cancellation during
setup or header staging returns `STYXRPC_IO_CANCELLED`. If a submitted
value fragment loses its acknowledgement, even during cancellation, the
SDK reports `STYXRPC_IO_UNCERTAIN`. The server only replaces a cell
after accepting a complete frame. A complete acknowledged write can
succeed despite a racing stop. The SDK sends the assignment once.
Releasing the operation fid preserves any replacement already applied.

Link the hosted C target `StyxRpc::launch`. The optional `stop(state)`
runs on the calling thread and should return nonzero to stop. Keep
`state` alive through return and avoid reentering the session from the hook.
NULL uses ordinary blocking waits. On multiplexed Styx, a pending
request can be flushed by tag. Temporary fid cleanup can continue after
return. A sequential session checks stop between exchanges.

`STYXRPC_IO_NOT_OPEN` indicates invalid input or an unavailable service.
`STYXRPC_IO_FAILED` reports a definite failure or invalid response.
`STYXRPC_IO_CANCELLED` reports a caller stop. An exchange without
reliable confirmation can report `STYXRPC_IO_UNCERTAIN`.

```c
const unsigned char replacement[] = {STYXRPC_NAMED_BOOLEAN, 0};
StyxRpcIoOutcome result = styxrpc_cell_write_until(session, token,
    replacement, sizeof(replacement), stop_requested, task);
```

The example uses `stop_requested(task)` to read the caller's stop flag.
After an uncertain result, [read](cell_read_until.md) if the application
needs the currently visible value. Other holders can write between
these operations. Serialize higher-level read/modify/write sequences
at the application level when necessary.
