# styxrpc_channel_group_reverse_borrow_graph

[Русский](channel_group_reverse_borrow_graph.ru.md) · [libstyxrpc](../README.md) · [Declaration](../include/styxrpc/channel_group.h#L51) · [Implementation](../src/channel_group.c#L277)

## Syntax

```c
#include <styxrpc/channel_group.h>
StyxRpcStatus styxrpc_channel_group_reverse_borrow_graph(StyxRpcChannelQueue *queues, size_t count,
    size_t root, StyxRpcChannelOutput *output);
```

Borrow a complete graph of FIFO and rendezvous channels for one reverse PID. Link `StyxRpc::serve`.

| Parameter | Contract |
| --- | --- |
| `queues` | Mutable contiguous descriptors. Each has a distinct positive 31-bit key, a complete parenthesized element schema, and capacity from 0 to 8 slots. Managed references and hybrid handles are outside this profile. |
| `count` | 1 through `STYXRPC_CHANNEL_SLOTS` descriptors. |
| `root` | Zero-based index whose key and capacity flag the resource returns to the caller. |
| `output` | Required result. On success it owns the resource through `release`. |

For each FIFO supply disjoint byte and length arrays. Set stride from 1 through `STYXRPC_MANAGED_LIMIT` bytes, head below capacity and count at most capacity. A queue with capacity zero creates a rendezvous and has zero cursors. Each occupied slot is a complete encoded element. Its channel leaves use current-bus keys from this group or nil. Each key must have the exact declared element type. A rendezvous leaf can arrive with the ordinary key marker and is emitted with the native rendezvous marker.

Returns `STYXRPC_OK` for a prepared resource, or `STYXRPC_INVALID` if metadata, frames, or allocation fail. Failure leaves both the descriptors and their arrays unchanged and preserves `output`.

Keep the descriptors and slot arrays exclusively lent through `output.release(output.resource.context)`. The resource uses native post-request reply offsets. Run its callbacks serially and retire every open fid before release. Register it with [program server grant](program_server_grant.md) and use [revocation](program_server_revoke.md) to close its fids first. By-value FIFOs use the caller's arrays directly. The SDK normalizes and stages nested frames in its own arenas, then returns their live slots and final cursors to the caller. Inspect remaining elements by the returned head and count.

The [standalone group test](../tests/channel_group.c) rejects an unknown child and checks that an echoed rendezvous survives release. For a single by-value queue see [reverse borrow](channel_group_reverse_borrow.md).
