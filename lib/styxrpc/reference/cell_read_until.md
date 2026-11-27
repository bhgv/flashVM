# styxrpc_cell_read_until

[Русский](cell_read_until.ru.md) · [Shared cells](named_cell.md) · [Declaration](../include/styxrpc/named_cell.h#L82) · [Implementation](../src/named_channel_client.c#L346)

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_read_until(
    struct StyxClient *session, const void *token, unsigned char **bytes,
    size_t *size, StyxRpcNamedStop stop, void *state);
```

Read a current [value frame](cell_value.md) from a shared cell through its
issuing `session`. Borrow a `STYXRPC_CELL_TOKEN`-byte `token` and keep
an existing lease or lifetime pin active through the call. The SDK
attaches and retires its own temporary fid on every outcome. The caller's
lease remains usable. `STYXRPC_IO_OK` transfers the malloc-owned frame
into `bytes`, with its length in `size`. Release the bytes with `free`.
Failure preserves both slots.

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
unsigned char *frame = NULL;
size_t size = 0;
StyxRpcIoOutcome result = styxrpc_cell_read_until(session, token, &frame,
    &size, stop_requested, task);
if (result == STYXRPC_IO_OK)
{
    free(frame);
}
```

The example uses `stop_requested(task)` to read the caller's stop flag.
The token comes from a live shared cell on this session. Decode the frame
before freeing it when using its value. Concurrent callers use separate
output slots and keep their shared lifetime pin stable.
