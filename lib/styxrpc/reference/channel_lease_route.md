# styxrpc_channel_lease_route

[Русский](channel_lease_route.ru.md) · [Declaration](../include/styxrpc/channel_client.h) · [Implementation](../src/channel_client.c) · [Lease guide](channel_leases.md)

Include `<styxrpc/channel_client.h>` and link `StyxRpc::launch`.

```c
const StyxRpcChannelRoute *styxrpc_channel_lease_route(const StyxRpcChannelLease *lease, size_t index);
```

Returns a borrowed read-only route in `lease` at the zero-based `index`, or NULL when the lease is NULL or the index falls outside the admitted routes. The returned route remains valid through that lease reference's lifetime. Calls perform no I/O and must use the same serialized owner as other lease operations.

The initial entries correspond to `StyxRpcLeasedReply.routes`. [Importing a received element](channel_lease_import_element.md) appends newly admitted routes. Iterate through the entries until NULL and use the desired index for [leased exchange](channel_lease_exchange.md).
