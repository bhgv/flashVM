# styxrpc_channel_lease_admit_module

[Русский](channel_lease_admit_module.ru.md) · [Declaration](../include/styxrpc/channel_client.h) · [Implementation](../src/channel_client.c) · [Lease guide](channel_leases.md)

Include `<styxrpc/channel_client.h>` and link `StyxRpc::launch`.

```c
StyxRpcStatus styxrpc_channel_lease_admit_module(StyxRpcChannelLease *lease,
    const StyxRpcChannelRoute *route, size_t *index);
```

Pass a live `lease`, an authorized `STYXRPC_ROUTE_MODULE` address in `route`, and non-null `index` storage. The host confirms that the input belongs to this particular issuing lease. For derived messages, check the transport, element schema and route before using this API. The SDK copies the address and binds operations to the original held module. No fresh path lookup or network I/O occurs here.

`STYXRPC_OK` writes the existing or newly appended zero-based route index. The registry merges identical token/key pairs only when types and capacity match. Invalid input, conflicts or exhaustion of the eight-route table return `STYXRPC_INVALID`, preserving the table and output index. Use the lease's serialized owner.

Use the returned index with [leased exchange](channel_lease_exchange.md). The operation uses a fresh fid, presents the exact token and retires the fid on completion. Clunk of the issuing function controls revocation. [Native C scenario](../examples/disvm/routed_client.c)
