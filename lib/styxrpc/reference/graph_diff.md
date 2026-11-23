# styxrpc_graph_diff

[Русский](graph_diff.ru.md) · [Declaration](../include/styxrpc/graph.h#L81) · [Implementation](../src/graph.c)

Link `StyxRpc::rpc` and include `styxrpc/graph.h`. Supply `schema`, the checked
original argument list, the original zero-based `index` and the corresponding
ref `type`. The complete `before` and `after` ref records share that original
ID. Provide disjoint scratch and output buffers. The function uses the
original argument layout to choose a positional selector or nested ID.
For type `h`, the definitions contain complete hybrid table snapshots and
the update contains a [key patch](hybrid_diff.md).

The function returns 1 and sets `used` to the extent of one sparse update, or
to zero if every encoded field is equal. Invalid input or insufficient space
returns 0. The caller owns the input records and both buffers.

[Resolve the original ID](graph_update_index.md) · [Read sparse fields](graph_update_fields.md)
