# styxrpc_module_live_open_until

[Русский](module_live_open_until.ru.md) · [Declaration](../include/styxrpc/module_client.h) ·
[Implementation](../src/live_module_client.c)

```c
#include <styxrpc/module_client.h>

typedef int (*StyxRpcModuleStop)(void *state);
StyxRpcIoOutcome styxrpc_module_live_open_until(struct StyxClient *session,
    const char *path, struct StyxFid **directory, struct StyxFid **lease,
    StyxRpcModuleStop stop, void *state);
```

Use the hosted `StyxRpc::launch` target. Borrow a mounted `session` and
a canonical root-relative `.dis` path as in
[`styxrpc_module_live_open`](module_live_open.md). Pass distinct writable
slots for the exact unopened directory and the request lease. On
`STYXRPC_IO_OK`, both slots receive owned fids. Keep the lease open
through every call and any live result channels. Close both with
`styxclientclunk` once their owners finish.

The optional `stop(state)` runs on the calling thread. It returns nonzero
to stop while opening the service, writing the path, waiting for
readiness or capturing the generation. Keep `state` alive through return.
A NULL hook uses ordinary blocking exchanges and readiness polling.

`STYXRPC_IO_CANCELLED` reports a stop. An invalid request or unavailable
control file returns `STYXRPC_IO_NOT_OPEN`. A refused image, invalid
publication or expired readiness wait returns `STYXRPC_IO_FAILED`.
An unacknowledged path write can return `STYXRPC_IO_UNCERTAIN`. Failure
preserves both outputs. A temporary request is retired when its clunk
reaches the provider. It may do so after return. The SDK never replays
the write.

Readiness polling has a four-second budget. A multiplexed session flushes
a pending walk, stat, open, read or write by tag. Flush acknowledgement and
cleanup use the session's deadlines. Call on a thread allowed to wait
for I/O. Separate calls use separate fids.

```c
struct StyxFid *directory = NULL;
struct StyxFid *lease = NULL;
StyxRpcIoOutcome outcome = styxrpc_module_live_open_until(session,
    "lib/counter.dis", &directory, &lease, stop_requested, task);
if (outcome == STYXRPC_IO_OK) {
    /* Discover and call the exact exports before releasing the two fids. */
    styxclientclunk(directory);
    styxclientclunk(lease);
}
```

Here `stop_requested(task)` checks the caller's stop flag. See
[export discovery](module_exports_at.md) and
[live loading in FlashVM](../../../flashVM/doc/STYX_RPC_LIVE_DIS.md).
