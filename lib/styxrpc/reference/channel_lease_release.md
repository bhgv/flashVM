# styxrpc_channel_lease_release

[Русский](channel_lease_release.ru.md) · [Declaration](../include/styxrpc/channel_client.h#L43) · [Implementation](../src/channel_client.c) · [Lease guide](channel_leases.md)

Link `StyxRpc::launch`.

```c
#include <styxrpc/channel_client.h>
void styxrpc_channel_lease_release(StyxRpcChannelLease *lease, int deferred);
```

Consume one owned `lease` reference. Null is accepted. The last release retires the function fid, which revokes the remote token, and retires its module fid. A zero `deferred` uses synchronous Styx clunk. A nonzero value schedules best-effort retirement via Styx's deferred cleanup, for instance from an owner-local finalizer.

Consume each reference at most once and serialize release with operations on the lease. Keep the native session active until a synchronous clunk when the application needs immediate remote retirement. Session teardown also invalidates outstanding capabilities.

[Executable fixture](../examples/disvm/routed_client.c)
