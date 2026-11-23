# styxrpc_scheduler_progress

[Русский](scheduler_progress.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_scheduler.h>
void styxrpc_scheduler_progress(StyxRpcChannelScheduler *scheduler);
```

Drive the supplied initialized `scheduler` until no other queued guard can
commit. The scheduler visits pending fids in admission order and their guards
in submitted order. On a synchronous queue it pairs distinct fids and encodes
both replies before either becomes readable. A buffered queue transfers one
element at a time in FIFO order. An undersized output buffer may fail the
affected operation. A failed transfer preserves the buffered value.

TRY completes with ordinal zero if idle. WAIT completes with ordinal zero
at a progress call at least 40 ms after admission if still idle. Call progress
periodically while a WAIT can be pending. ALT and single blocking operations can
remain pending until a match or clunk. The function returns no value, performs
no I/O and owns no background thread. Run it on the serialized owner.
Link `StyxRpc::rpc`.

[styxrpc_scheduler_init](scheduler_init.md) · [Example](../tests/scheduler.c) · [Source](../src/channel_scheduler.c)
