# styxrpc_channel_lease_retain

[Русский](channel_lease_retain.ru.md) · [Declaration](../include/styxrpc/channel_client.h#L38) · [Implementation](../src/channel_client.c) · [Lease guide](channel_leases.md)

Link `StyxRpc::launch`.

```c
#include <styxrpc/channel_client.h>
int styxrpc_channel_lease_retain(StyxRpcChannelLease *lease);
```

Take another reference to the live `lease`, typically before sharing it with another owner-local object. Return 1 on success and 0 for null or counter exhaustion.

Serialize retain, exchange and release for this lease on one application owner. Balance each successful retain with [release](channel_lease_release.md). This operation performs no transport I/O.

[Executable fixture](../examples/disvm/routed_client.c)
