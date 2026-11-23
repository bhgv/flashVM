# styxrpc_graph_update_fields

[Русский](graph_update_fields.ru.md) · [Declaration](../include/styxrpc/graph.h#L76) · [Implementation](../src/graph.c)

Link `StyxRpc::rpc` and include `styxrpc/graph.h`. Read one UniBus record from
the sparse list in a checked [graph reply](graph_read.md). Pass it as `update`
with the original `r[…]` type. On success the function returns 1 and writes
the selected flattened type list, bit mask and encoded children to `fields`.
An invalid update returns 0.

The returned child record borrows bytes from the update. Keep the reply
buffer intact while using that view. The codec uses no VM owner or heap.

[Resolve the original ID](graph_update_index.md) · [Encode a change](graph_diff.md)
