# styxrpc_channel_lease_exchange

[Русский](channel_lease_exchange.ru.md) · [Declaration](../include/styxrpc/channel_client.h#L42) · [Implementation](../src/channel_client.c) · [Lease guide](channel_leases.md)

Link `StyxRpc::launch`.

```c
#include <styxrpc/channel_client.h>
StyxRpcStatus styxrpc_channel_lease_exchange(StyxRpcChannelLease *lease, size_t route_index,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result);
```

Use the original mounted module in `lease`, with the token of zero-based `route_index` in the copied result routes. `selection` may use the keys originally issued under that token. Keep the lease and selection alive throughout. Set `timeout_ms` to 1–60000. Pass nonoverlapping scratch, request and reply buffers with their byte capacities. Scratch is needed for guard wrappers.

`STYXRPC_OK` writes a validated `result`, borrowing the reply's typed bytes when present. `STYXRPC_MORE` signals an expired polling budget, `STYXRPC_REMOTE_ERROR` an explicit refusal, and `STYXRPC_INVALID` an invalid request, unavailable route, transport or framing failure. The operation opens and clunks a fresh fid. Timing is checked between synchronous transactions. Report an uncertain operation without automatically replaying it.

[Executable fixture](../examples/disvm/routed_client.c)
