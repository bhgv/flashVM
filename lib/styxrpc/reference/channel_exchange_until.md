# styxrpc_channel_exchange_until

[Русский](channel_exchange_until.ru.md) · [Declaration](../include/styxrpc/channel_call.h#L83) · [Implementation](../src/channel_exchange.c#L214)

```c
#include <styxrpc/channel_call.h>

StyxRpcStatus styxrpc_channel_exchange_until(struct StyxClient *reverse, const StyxRpcChannelCall *call,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity,
    StyxRpcChannelResult *result, StyxRpcCallStop stop, void *state);
```

Link the hosted `StyxRpc::launch` target. This call uses the same
selection and route contract as [`styxrpc_channel_exchange`](channel_exchange.md).

Borrow the reverse attachment and the checked call with its caller PID
and admitted keys. Keep the active call live through the operation.

Set `timeout_ms` from 1 to 60000 milliseconds. Supply disjoint scratch,
request and reply buffers and their byte capacities. Scratch is needed
when the selection packs guard wrappers. On `STYXRPC_OK`, `result`
contains the selected ordinal and can borrow typed bytes from `reply`.

The optional [`StyxRpcCallStop`](../include/styxrpc/rpc.h) callback
receives borrowed `state` on the calling thread. Return a nonzero value
to stop. The SDK checks during the bus walk, open, any required token
authentication and the data exchange. A multiplexed session flushes a
pending request by tag. Flush acknowledgement and cleanup use the
session's deadlines. A NULL callback gives the ordinary behavior.

| Outcome | Meaning |
| --- | --- |
| `STYXRPC_CANCELLED` | The stop was observed before a guard write. Token authentication alone is allowed. |
| `STYXRPC_CANCELLED_UNCERTAIN` | The stop was observed during or after a guard write, and an admitted branch may have taken effect. |

Both cancellation outcomes retire the exact operation fid and preserve
`result`. If no guard was sent, cleanup can continue after return.
A complete reply read before the stop keeps its ordinary outcome. No request is replayed. The original route remains borrowed.
Timeout, explicit refusal and malformed exchange retain the ordinary API
statuses. Use distinct buffers for concurrent operations and serialize
updates of any shared route table.

```c
StyxRpcStatus status = styxrpc_channel_exchange_until(
    reverse, call, &selection, 1000, scratch, sizeof(scratch),
    request, sizeof(request), reply, sizeof(reply), &result, stop_requested, task);
```

In this example `stop_requested(task)` reads the caller's stop flag.
See the [paired acceptance](../../../flashVM/doc/STYX_RPC_LIVE_DIS.md),
which provides the `--cancel-dis-channel` scenario.
