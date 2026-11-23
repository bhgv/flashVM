# styxrpc_scheduler_cancel

[Русский](scheduler_cancel.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_scheduler.h>
void styxrpc_scheduler_cancel(void *context, StyxRpcChannelFid *fid);
```

Install as the `cancel` callback with the scheduler as `context`. The normal
close path calls it with the unresolved `fid` before releasing its buffers.
The scheduler removes that exact pointer from the pending list. A completed
or absent fid leaves the scheduler unchanged. A committed buffered transfer
keeps its effect.

The callback returns no value and uses no I/O. Invoke on the scheduler owner.
Link `StyxRpc::rpc`.

[styxrpc_channel_fid_close](channel_fid_close.md) · [Example](../tests/scheduler.c) · [Source](../src/channel_scheduler.c)
