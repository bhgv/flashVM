# styxrpc_module_slot_new_node_at

[Русский](module_slot_new_node_at.ru.md) · [Header](../include/styxrpc/nodes.h) · [Implementation](../src/module_slot.c)

```c
#include <styxrpc/nodes.h>
StyxRpcStatus styxrpc_module_slot_new_node_at(StyxRpcModuleSlot *slot,
    const char *path, unsigned mode, uint64_t length,
    const StyxRpcPositionedNodeCallbacks *callbacks);
```

Link the hosted SDK. Publish into the live `slot` on its owner thread.
The `path`, Unix `mode`, return codes and lifetime follow
[styxrpc_module_slot_new_node](module_slot_new_node.md). `callbacks`
supplies a transfer function, optional release function and context.
The SDK copies the table and adopts the context on success when a
release function is supplied. `length` is a byte count from zero
through `STYXRPC_NODE_OFFSET_MAX` (2⁵³−1). The size appears in stat
and does not imply backing storage.

The transfer receives `context`, the access selector, the absolute
`offset` in bytes, a borrowed buffer and byte `size`. Supply `*used`
between zero and `size` and return one to confirm that prefix.
A read past EOF returns zero without dispatch. Reads overlapping EOF
receive a smaller buffer. A write fragment crossing EOF is declined
without dispatch. Write data is read-only. Zero-byte data I/O
confirms zero without entering the handler. A complete execute request
is independent of file length and receives offset zero.

For example, `styxrpc_module_slot_new_node_at(slot, "app/page", 4, 4096, &ops)`
publishes a 4096-byte readable file using the transfer function in
`ops`. A read at byte 4095 offers at most one byte. Call
[clear_nodes](module_slot_clear_nodes.md) on the slot owner to release it.
