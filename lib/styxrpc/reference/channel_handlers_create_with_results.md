# styxrpc_channel_handlers_create_with_results

[Русский](channel_handlers_create_with_results.ru.md) · [Declaration](../include/styxrpc/channel_handlers.h) · [Queue implementation](../src/channel_handlers.c) · [Styx files](../src/channel_handlers_files.c) · [Guide](channel_handler_modules.md)

Include `<styxrpc/channel_handlers.h>` and link the hosted C99 target `StyxRpc::serve` with POSIX threads.

```c
StyxRpcChannelHandlers *styxrpc_channel_handlers_create_with_results(const char *name,
    const StyxRpcChannelFunction *functions, size_t count, const StyxRpcChannelExport *results,
    size_t result_count, StyxRpcModuleChannels *registry);
```

| Parameter | Contract |
| --- | --- |
| `name` | Required module component under `mounted`. Provide a nonempty name of up to 255 bytes, excluding `.`, `..` and slash characters. |
| `functions`, `count` | [Copied-result functions](channel_handlers_create.md) and their count. Use NULL and zero to omit this class of export. |
| `results`, `result_count` | Functions with channel positions in their result, and their count. Each descriptor contains the exact compiled name, signature and schema, a `begin` callback and its context. Inputs may include borrowed channels. Argument elements can carry current-bus channels. The result channel can carry nested keys on the same [group resource](channel_group_output.md). Use NULL and zero to omit this class. |
| `registry` | [Initialized lease registry](module_channels_init.md), required when `result_count` is positive. Reserve slots for the simultaneous result lifetimes. The dispatcher exclusively uses the registry and its entropy callback. |

The total export count must be from 1 to `STYXRPC_MAX_EXPORTS`. Names must be unique across both arrays. Returns the owned module or NULL for invalid descriptors or unavailable memory or synchronization resources. The module borrows both arrays, names, callback contexts and registry through shutdown.

Serve `styxrpc_channel_handlers_tree` from a single dispatcher on the forward session. Call [step](channel_handlers_step.md) on the callback owner. A `begin` callback supplies a resource, positive key, capacity flag and optional release callback in `StyxRpcChannelOutput`. A zero key sends a nil channel. Keep the resource usable after callback return. Resource I/O and revocation run on the dispatcher. The release callback may run on either the owner or dispatcher and should release the transferred resource once, including after a failed or abandoned call.

The dispatcher registers a token on the first nonempty read from offset zero after callback completion. It exposes the token only in the verified reply and accepts it on `.unibus`. Clunk of the issuing fid revokes its operations before releasing the resource. A call abandoned before publication consumes no token. If publication fails, the dispatcher releases the resource and produces a remote error. The caller can still observe completed operations on its argument channels.

[Standalone C scenario](../tests/module_channels.c) · [Stop](channel_handlers_stop.md) · [Destroy](channel_handlers_destroy.md)

A descriptor with a leased channel result can accept copied graph arguments and borrowed channels in the same call. The `StyxRpcChannelOutput` update fields follow the [channel output ownership contract](channel_module_init.md). A copied-result descriptor uses its ordinary value result API.

Composite results use the [same output contract](channel_module_init.md#composite-replies).
