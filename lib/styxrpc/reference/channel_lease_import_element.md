# styxrpc_channel_lease_import_element

[Русский](channel_lease_import_element.ru.md) · [Declaration](../include/styxrpc/channel_client.h) · [Implementation](../src/channel_client.c) · [Lease guide](channel_leases.md)

Include `<styxrpc/channel_client.h>` and link `StyxRpc::launch`.

```c
StyxRpcStatus styxrpc_channel_lease_import_element(StyxRpcChannelLease *lease, size_t route_index,
    const void *bytes, size_t size);
```

| Argument | Meaning |
| --- | --- |
| `lease` | Live result lease on its serialized application owner. |
| `route_index` | Zero-based admitted route used in the confirmed receive. Its token and element schema determine where new keys belong. |
| `bytes` | Complete typed element from the confirmed receive, typically `result.value` after [leased exchange](channel_lease_exchange.md). Borrowed for validation. |
| `size` | Exact element length in bytes, bounded by `STYXRPC_MANAGED_LIMIT`. |

Returns `STYXRPC_OK` after merging checked current-bus keys into the parent token's namespace. The SDK copies exact types and capacity flags, preserves existing indices and merges matching aliases. A malformed record, separate PID or token, conflicting alias or full table returns `STYXRPC_INVALID` with the old lease intact. The bound counts all admitted routes up to `STYXRPC_CHANNEL_SLOTS`.

This function performs no network I/O. Supply data only from a successful selected receive on that exact route. Use [the route accessor](channel_lease_route.md) to inspect the resulting entries and [leased exchange](channel_lease_exchange.md) to use them. Keep the lease alive for as long as any nested key is in use. For a borrowed call, use [its scoped import](channel_import_element.md).
