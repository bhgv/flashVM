# styxrpc_scheduler_admit

[Русский](scheduler_admit.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_scheduler.h>
int styxrpc_scheduler_admit(void *context, StyxRpcChannelFid *fid);
```

Install as the `admit` callback for an opened channel `fid`, with the configured
scheduler as `context`. The fid calls it once after validating and staging a
complete operation. It compares every guard key and its element type with the
registered queues and attempts to commit a ready operation.

Returns 1 if accepted, including immediate completion. Returns 0 if the queue
of pending operations is full, a guard is unregistered or has the wrong shape,
the fid is already queued, or WAIT has no clock. A queued fid and its buffers
remain borrowed until completion or cancellation. The scheduler never blocks
and performs no I/O. Run all callbacks and progress on the serialized owner.
Link `StyxRpc::rpc`.

[styxrpc_scheduler_cancel](scheduler_cancel.md) · [Example](../examples/disvm/owned_port.c) · [Source](../src/channel_scheduler.c)
