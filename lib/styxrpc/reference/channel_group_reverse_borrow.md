# styxrpc_channel_group_reverse_borrow

[Русский](channel_group_reverse_borrow.ru.md) · [libstyxrpc](../README.md) · [Declaration](../include/styxrpc/channel_group.h#L42) · [Implementation](../src/channel_group.c#L224)

## Syntax

```c
#include <styxrpc/channel_group.h>
StyxRpcStatus styxrpc_channel_group_reverse_borrow(StyxRpcChannelQueue *queue, StyxRpcChannelOutput *output);
```

Expose an existing by-value FIFO or rendezvous through a native reverse PID. `queue` carries a positive 31-bit key, a complete parenthesized element schema, and a capacity of 0 to 8 slots. Capacity zero selects rendezvous and requires zero cursors. For a FIFO, provide stable `bytes` and `sizes` arrays, a byte stride between 1 and `STYXRPC_MANAGED_LIMIT`, a head index below capacity and a count at most capacity. Every occupied slot must contain a complete encoded element and its length in bytes. Use a value schema without channels, managed references or hybrid handles.

On `STYXRPC_OK`, the required `output` owns a reverse `.unibus` resource through its `release` callback and reports its key and rendezvous flag. The resource works on the supplied byte and length storage with native post-request read offsets. On invalid input, invalid frames or allocation failure the call returns `STYXRPC_INVALID`, leaving both descriptors unchanged.

Keep the queue descriptor and its storage exclusively lent until `output.release(output.resource.context)`. Serialize operations on the dispatcher. Publish through [program server grant](program_server_grant.md) and [revoke it](program_server_revoke.md) before releasing. Release returns the advanced head and count to the original descriptor. Queued data stays in the caller's arrays. Link `StyxRpc::serve`. The [group test](../tests/channel_group.c) verifies the returned cursor and the same backing memory.
