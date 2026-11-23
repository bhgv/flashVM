# styxrpc_channel_module_init

[Русский](channel_module_init.ru.md) · [Declaration](../include/styxrpc/channel_module.h#L46) · [Implementation](../src/channel_module.c) · [Lease guide](channel_leases.md)

Link `StyxRpc::serve`.

```c
#include <styxrpc/channel_module.h>
StyxRpcStatus styxrpc_channel_module_init(StyxRpcChannelModule *module, const char *name,
    const StyxRpcChannelExport *exports, size_t count, StyxRpcModuleChannels *registry);
```

Prepare application-owned `module` storage using the basename `name`, `count` distinct compiled descriptors in `exports` and an initialized per-module, per-session `registry`. Each descriptor accepts by-value input and has exactly one result channel with by-value elements. Serve `module` as the context of `styxrpc_channel_module_tree`. The module publishes the compiled offer, native capability markers and an authenticated `.unibus`.

`STYXRPC_OK` means the module is ready. `STYXRPC_INVALID` rejects invalid configuration. Keep the module, names, descriptors, registry and its entropy source alive through server destruction. Callbacks execute serially on the dispatcher. A successful callback may return a positive key and resource for a new lease, or zero for nil. If it supplies `release`, cleanup calls it once when the publication fails or is retired. Revocation closes pending operation fids before calling that cleanup.

[Executable fixture](../tests/module_channels.c)
