# styxrpc_module_slot_new_node

[Русский](module_slot_new_node.ru.md) · [Header](../include/styxrpc/nodes.h) · [Implementation](../src/module_slot.c)

```c
#include <styxrpc/nodes.h>
StyxRpcStatus styxrpc_module_slot_new_node(StyxRpcModuleSlot *slot,
    const char *path, unsigned mode, const StyxRpcNodeCallbacks *callbacks);
```

Link the hosted SDK. Supply a live module slot, a path relative to the
session root with an optional leading slash, and a Unix permission mask
from 1 through 7. Each component may be at most 255 bytes and the full
path up to 1024 bytes. The limit is 128 new files and implicit directories
combined. Reserved names, conflicting nodes and invalid paths return
`STYXRPC_INVALID`. On success `STYXRPC_OK` publishes the whole path and
copies callbacks. If supplied, release takes ownership of context for
later withdrawal. A failed call leaves the caller's context intact.

The transfer callback runs synchronously on the slot owner with
`STYXRPC_READ` or `STYXRPC_WRITE`. Answer by writing a read prefix or
examining a write input and setting `used` to a count up to `size`.
The borrowed bytes are valid for the callback only. Treat write input as
read-only. Return 1 to acknowledge the prefix, or 0 to reject the operation.
With execute permission the selector is `STYXRPC_NODE_EXECUTE`. The borrowed
buffer contains the request on entry. Fill its leading bytes with the reply
and set `used` to its length, up to `size`. The complete operation has one
callback through [node_exec](node_exec.md). Use [clear_nodes](module_slot_clear_nodes.md)
to retire the publication.

For fixed length and byte offsets, use
[styxrpc_module_slot_new_node_at](module_slot_new_node_at.md).
