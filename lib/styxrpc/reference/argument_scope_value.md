# styxrpc_argument_scope_value

[Русский](argument_scope_value.ru.md) · [Declaration](../include/styxrpc/channel_arguments.h#L41) · [Implementation](../src/channel_arguments.c) · [Scope guide](channel_argument_scopes.md)

Link `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
StyxRpcStatus styxrpc_argument_scope_value(StyxRpcArgumentScope *scope, StyxRpcChannelQueue *queue,
    void *bytes, size_t capacity, size_t *used);
```

Pass the inactive staging `scope` and caller-owned `queue`, with the exact one-element parenthesized shape in its grant, byte arena, size table, slot count, stride and valid cursors. Queue identity is its address. Multiple occurrences get the same current-bus key within the scope. The scope admits up to eight distinct queues. Supply caller-owned output `bytes` of `capacity` bytes. `STYXRPC_OK` reports the written extent in `used`. Invalid input, exhausted keys or insufficient storage gives `STYXRPC_INVALID`. Keep the queue and storage stable and exclusive to the active call until retirement.

[C regression](../tests/channel_arguments.c)
