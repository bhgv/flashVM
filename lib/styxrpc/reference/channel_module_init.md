# styxrpc_channel_module_init

[Русский](channel_module_init.ru.md) · [Declaration](../include/styxrpc/channel_module.h#L50) · [Implementation](../src/channel_module.c) · [Lease guide](channel_leases.md)

Link `StyxRpc::serve`.

```c
#include <styxrpc/channel_module.h>
StyxRpcStatus styxrpc_channel_module_init(StyxRpcChannelModule *module, const char *name,
    const StyxRpcChannelExport *exports, size_t count, StyxRpcModuleChannels *registry);
```

Prepare application-owned `module` storage using the basename `name`, `count` distinct compiled descriptors in `exports` and an initialized per-module, per-session `registry`. Each descriptor accepts copied and graph arguments and a typed result that contains channels. Use the owner-driven handlers for borrowed channel arguments. The callback may issue [a group of queues](channel_group_output.md) with nested channels in its messages. Serve `module` as the context of `styxrpc_channel_module_tree`. The module publishes the compiled offer, native capability markers and an authenticated `.unibus`.

`STYXRPC_OK` means the module is ready. `STYXRPC_INVALID` rejects invalid configuration. Keep the module, names, descriptors, registry and its entropy source alive through server destruction. Callbacks execute serially on the dispatcher. A successful callback may return a positive key and resource for a new lease, or zero for nil. If it supplies `release`, cleanup calls it once when the publication fails or is retired. Revocation closes pending operation fids before calling that cleanup.

[Executable fixture](../tests/module_channels.c)

A callback can set `StyxRpcChannelOutput.updates` to a `malloc`-owned buffer of positional update children and `update_size` to their byte count. The SDK takes ownership even if the callback fails or the dispatcher cannot publish the reply. Leave both fields zero for no changes. Use [`styxrpc_graph_diff`](graph_diff.md) for changed fields. The SDK wraps the updates and validates the result and changes together. Rebuild providers with the current structure definition.

### Composite replies

For the implicit single channel set `key` and `unbuffered`. For a composite result transfer a separately `malloc`-allocated `result` buffer holding one encoded child and its byte length in `result_size`. Encode every live channel position using [`styxrpc_channel_route_value`](channel_route_value.md) with `STYXRPC_ROUTE_CURRENT`. The resource serves those exact keys, types and capacities. `key` is the first live key in result order or zero if all channel positions are nil. The SDK takes both the result and update buffers on callback return, including on error. It stamps its token and checks aliases and graph changes together before exposing the reply. On rejection it revokes any staged token and releases the resource. Rebuild C providers with the current `StyxRpcChannelOutput`.

[Runnable C, Limbo and AS2 example](../examples/disvm/CHANNEL_RESULTS.md).
