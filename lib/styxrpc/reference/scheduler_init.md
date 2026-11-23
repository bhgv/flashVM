# styxrpc_scheduler_init

[Русский](scheduler_init.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_scheduler.h>
int styxrpc_scheduler_init(StyxRpcChannelScheduler *scheduler, StyxRpcChannelQueue *queues,
    size_t count, uint64_t (*clock_ms)(void *), void *context);
```

Initialize a scheduler over `count` queues, from 1 through `STYXRPC_CHANNEL_SLOTS`.
Each fresh queue has a distinct positive key and its exact declared element shape
in parentheses. Zero head and count. A zero `capacity` selects synchronous
rendezvous and uses no arena. A positive capacity requires `bytes` of
`capacity * stride` bytes and `sizes` with one length per slot. Each stride
holds one complete sending element and is bounded by `STYXRPC_MANAGED_LIMIT`.
Keep entries and their nonoverlapping storage stable for the scheduler's lifetime.

Supply a monotonic millisecond `clock_ms` callback and its borrowed `context`
to support WAIT. Pass `NULL` for the callback if WAIT is unused. The scheduler
and its fids must share one serialized owner and one channel authority scope.
Use separate instances for unrelated routes or leases. For every fid in a
shared instance, each key must refer to the same channel throughout the lifetime
of queued elements.

Returns 1 on success and 0 for invalid configuration. No I/O or allocation.
Initialize a fresh scheduler or retire its pending fids first. Link `StyxRpc::rpc`.

[styxrpc_scheduler_admit](scheduler_admit.md) · [Example](../tests/scheduler.c) · [Source](../src/channel_scheduler.c)
