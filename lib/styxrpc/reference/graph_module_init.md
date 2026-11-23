# styxrpc_graph_module_init

[Русский](graph_module_init.ru.md) · [Declaration](../include/styxrpc/graph_module.h#L49) · [Implementation](../src/graph_module.c#L25)

Link `StyxRpc::serve` and include `styxrpc/graph_module.h`. Pass caller-owned module storage, a valid module basename and an immutable array of one or more [graph export descriptors](../include/styxrpc/graph_module.h#L28). The initializer validates the compiled offers and distinct names before publishing the module state. `STYXRPC_OK` means the module is ready. `STYXRPC_INVALID` means the arguments or compiled profiles are invalid.

Pass the module as the context for [`styxrpc_graph_module_tree`](../include/styxrpc/graph_module.h#L52) when creating the server. Keep the module, names and export descriptors alive through server teardown. Each opened function fid owns a bounded staging buffer and a separate immutable reply.

The owner callback borrows the validated argument record during the call and returns encoded result and sparse-update children. The callback's output buffers must remain readable until the dispatcher encodes them immediately after it returns. A void result and an empty update list each use zero children. Return `STYXRPC_OK` to publish validated output or `STYXRPC_REMOTE_ERROR` to report failure to the caller. Calls are synchronous on the Styx server owner. Serve a graph module with the exact native Dis positional marker. [The FlashVM graph exporter uses this module interface](../../../flashVM/doc/STYX_RPC_EXPORT_GRAPH.md).
