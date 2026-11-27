# styxrpc_cell_attach_until

[Русский](cell_attach_until.ru.md) · [Shared cells](named_cell.md) · [Declaration](../include/styxrpc/named_cell.h#L74) · [Implementation](../src/named_channel_client.c#L283)

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_attach_until(
    struct StyxClient *session, const void *token, StyxRpcVfsFile **out,
    StyxRpcNamedStop stop, void *state);
```

Attach to a borrowed `STYXRPC_CELL_TOKEN`-byte `token` on the issuing
`session`. Keep the received descriptor's transfer pin or another lease
live through the call. `STYXRPC_IO_OK` transfers a new read/write fid
to `out`. All other outcomes leave the slot unchanged. The new fid
shares the value and lifespan of the cell with other holders. Release it
with [styxrpc_vfs_close](vfs_close.md). A foreign or retired token is
refused. Outstanding operations on a named channel may replace its
transfer pin, so finish attaching before continuing on the channel.

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
StyxRpcVfsFile *alias = NULL;
StyxRpcIoOutcome result = styxrpc_cell_attach_until(session, token, &alias,
    stop_requested, task);
```

The example uses `stop_requested(task)` to read the caller's stop flag.
Use the token from a received named cell on the same attachment. After
success use [get](cell_get.md) and [set](cell_set.md) on the new lease, or
keep it open while using temporary I/O by token.
