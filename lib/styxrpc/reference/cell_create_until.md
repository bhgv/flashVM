# styxrpc_cell_create_until

[Русский](cell_create_until.ru.md) · [Shared cells](named_cell.md) · [Declaration](../include/styxrpc/named_cell.h#L66) · [Implementation](../src/named_channel_client.c#L240)

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_create_until(
    struct StyxClient *session, const void *frame, size_t size,
    void *token, StyxRpcVfsFile **out, StyxRpcNamedStop stop, void *state);
```

Create a shared cell on attached `session`. The peer must serve the shared
cell profile over `/.channels`. Borrow an accepted [value frame](cell_value.md)
of `size` bytes. Supply writable storage for `STYXRPC_CELL_TOKEN` bytes
at `token` and a distinct output pointer at `out`. `STYXRPC_IO_OK` publishes
the complete token and an owned read/write fid simultaneously from the
caller's perspective. Keep the fid open to retain the cell and close it with
[styxrpc_vfs_close](vfs_close.md). Both outputs retain their prior contents
on failure. If a cancelled creation reached the server, it remains leased
only until the temporary fid is retired.

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
unsigned char token[STYXRPC_CELL_TOKEN];
const unsigned char initial[] = {STYXRPC_NAMED_BOOLEAN, 1};
StyxRpcVfsFile *owner = NULL;
StyxRpcIoOutcome result = styxrpc_cell_create_until(session, initial,
    sizeof(initial), token, &owner, stop_requested, task);
```

The example uses `stop_requested(task)` to read the caller's stop flag.
Only after success, forward the token through the named cell descriptor or
use [read](cell_read_until.md) on the same session. Close the owned fid
after the last use by this caller.
