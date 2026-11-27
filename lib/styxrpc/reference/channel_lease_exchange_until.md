# styxrpc_channel_lease_exchange_until

[Русский](channel_lease_exchange_until.ru.md) · [Declaration](../include/styxrpc/channel_client.h#L55) · [Implementation](../src/channel_client.c#L201)

```c
#include <styxrpc/channel_client.h>

StyxRpcStatus styxrpc_channel_lease_exchange_until(StyxRpcChannelLease *lease,
    size_t route_index,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity,
    StyxRpcChannelResult *result, StyxRpcCallStop stop, void *state);
```

Link the hosted `StyxRpc::launch` target. This call uses the same
selection and route contract as [`styxrpc_channel_lease_exchange`](channel_lease_exchange.md).

Borrow the checked result lease. The zero-based `route_index` selects its
module token. Guards may use keys issued under that token. The operation
opens a temporary fid under the pinned module, and cancellation leaves the
issuing function fid and its lease valid.

Set `timeout_ms` from 1 to 60000 milliseconds. Supply disjoint scratch,
request and reply buffers and their byte capacities. Scratch is needed
when the selection packs guard wrappers. On `STYXRPC_OK`, `result`
contains the selected ordinal and can borrow typed bytes from `reply`.

The optional [`StyxRpcCallStop`](../include/styxrpc/rpc.h) callback
receives borrowed `state` on the calling thread. Return a nonzero value
to stop. The SDK samples it before opening and while waiting on a
data request. On a multiplexed session, a transmitted request gets one
`Tflush` by tag. Metadata, the flush acknowledgement and clunk use
the session's deadlines. A NULL callback gives the ordinary behavior.

| Outcome | Meaning |
| --- | --- |
| `STYXRPC_CANCELLED` | The stop was observed before a guard write. Token authentication alone is allowed. |
| `STYXRPC_CANCELLED_UNCERTAIN` | The stop was observed during or after a guard write, and an admitted branch may have taken effect. |

Both cancellation outcomes close the exact temporary operation fid and
preserve `result`. A complete reply read before the stop keeps its ordinary
outcome. No request is replayed. The original route remains borrowed.
Timeout, explicit refusal and malformed exchange retain the ordinary API
statuses. Use distinct buffers for concurrent operations and serialize
updates of any shared route table.

```c
StyxRpcStatus status = styxrpc_channel_lease_exchange_until(
    lease, route_index, &selection, 1000, scratch, sizeof(scratch),
    request, sizeof(request), reply, sizeof(reply), &result, stop_requested, task);
```

In this example `stop_requested(task)` reads the caller's stop flag.
See the [paired acceptance](../../../flashVM/doc/STYX_RPC_LIVE_DIS.md),
which provides the `--cancel-dis-channel` scenario.
