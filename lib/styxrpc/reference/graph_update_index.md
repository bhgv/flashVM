# styxrpc_graph_update_index

[Русский](graph_update_index.ru.md) · [Declaration](../include/styxrpc/graph.h#L70) · [Implementation](../src/graph.c)

Link `StyxRpc::rpc` and include `styxrpc/graph.h`. Pass the original `schema`,
the arguments from [`styxrpc_graph_arguments`](graph_arguments.md), one checked sparse
update and the number of defined request identities in `originals`. The update
may select a direct argument position or a nested ID. On success the function
returns 1 and stores the zero-based original identity in `index`. It returns 0
on an invalid selector or missing original.

Keep the input records and their backing storage alive during the call.
The codec operates on records independently of either VM.

[Read sparse fields](graph_update_fields.md) · [Encode a change](graph_diff.md)
