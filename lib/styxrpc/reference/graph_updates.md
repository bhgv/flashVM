# Positional graph updates

[Русский](graph_updates.ru.md) · [Declarations](../include/styxrpc/graph.h#L60) · [Implementation](../src/graph.c)

Link `StyxRpc::rpc` for the portable graph codec. Obtain the arguments through [`styxrpc_graph_arguments`](graph_arguments.md) and the update list through [`styxrpc_graph_read`](graph_read.md) or the hosted [graph client](graph_invoke.md). Read the list's individual records as UniBus S-records.

| Function | Inputs and output |
| --- | --- |
| [`styxrpc_graph_update_index`](graph_update_index.md) | Resolve a checked update to a zero-based ID in the checked original arguments, with `originals` as the number of existing IDs. |
| [`styxrpc_graph_update_fields`](graph_update_fields.md) | Given the original `r[…]` type and a checked update, return the sparse field mask, flattened selected type list and borrowed child records. |
| [`styxrpc_graph_diff`](graph_diff.md) | Given an original identity, its type and complete ref records before and after, encode one sparse update using separate scratch and output buffers. On equality, return success with `used=0`. |

All three return 1 on success and 0 for invalid input or insufficient capacity. The caller owns and retains the source buffers while borrowing their views. The layer operates on records and has no VM owner or heap dependency.
