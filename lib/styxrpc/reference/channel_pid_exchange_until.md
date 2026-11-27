# styxrpc_channel_pid_exchange_until

[Русский](channel_pid_exchange_until.ru.md) · [Declaration](../include/styxrpc/channel_route.h#L127) · [Implementation](../src/channel_pid.c#L148)

```c
#include <styxrpc/channel_route.h>

StyxRpcStatus styxrpc_channel_pid_exchange_until(struct StyxClient *reverse,
    StyxRpcChannelRoutes *routes, size_t anchor,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity,
    StyxRpcChannelResult *result, StyxRpcCallStop stop, void *state);
```

Link the hosted `StyxRpc::launch` target. This call uses the same
selection and route contract as [`styxrpc_channel_pid_exchange`](channel_pid_exchange.md).

Borrow the reverse attachment and the admitted route table. Select its
zero-based `anchor` to choose the PID. The selection uses keys of that PID.
A successful typed receive can admit nested keys into `routes`.
Cancellation leaves the table and its indices unchanged.

Set `timeout_ms` from 1 to 60000 milliseconds. Supply disjoint scratch,
request and reply buffers and their byte capacities. Scratch is needed
when the selection packs guard wrappers. On `STYXRPC_OK`, `result`
contains the selected ordinal and can borrow typed bytes from `reply`.

The optional [`StyxRpcCallStop`](../include/styxrpc/rpc.h) callback
receives borrowed `state` on the calling thread. Return a nonzero value
to stop. The SDK samples it before opening the operation and between
Styx round trips. A NULL callback leaves the usual deadline in effect.
An in-flight round trip and clunk use the mounted session's deadlines.

| Outcome | Meaning |
| --- | --- |
| `STYXRPC_CANCELLED` | The stop was observed before a guard write. Token authentication alone is allowed. |
| `STYXRPC_CANCELLED_UNCERTAIN` | The stop was observed after a guard write, and an admitted branch may have taken effect. |

Both cancellation outcomes close the exact temporary operation fid and
preserve `result`. A complete reply read before the stop keeps its ordinary
outcome. No request is replayed. The original route remains borrowed.
Timeout, explicit refusal and malformed exchange retain the ordinary API
statuses. Use distinct buffers for concurrent operations and serialize
updates of any shared route table.

```c
StyxRpcStatus status = styxrpc_channel_pid_exchange_until(
    reverse, routes, anchor, &selection, 1000, scratch, sizeof(scratch),
    request, sizeof(request), reply, sizeof(reply), &result, stop_requested, task);
```

In this example `stop_requested(task)` reads the caller's stop flag.
See the [paired acceptance](../../../flashVM/doc/STYX_RPC_LIVE_DIS.md),
which provides the `--cancel-dis-channel` scenario.
