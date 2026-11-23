# styxrpc_scalar_module_init

[Русский](scalar_module_init.ru.md) · [Header and tree](../include/styxrpc/scalar_module.h) · [Implementation](../src/scalar_module.c)

Include `styxrpc/scalar_module.h` and link `StyxRpc::serve` from the hosted SDK.

```c
StyxRpcStatus styxrpc_scalar_module_init(StyxRpcScalarModule *module, const char *name,
    const StyxRpcExport *exports, size_t count);
```

Prepare caller-owned `module` for publication at `mounted/<name>`. Supply a
nonempty path component of up to 255 bytes and 1–8 distinct scalar descriptors.
The descriptors contain export names, compiled signatures, callbacks and
contexts. The module owns its encoded `.interface` and borrows the names and
descriptors. `STYXRPC_OK` makes it ready for publication. On
`STYXRPC_INVALID`, do not serve the failed initialization.

Pass the module as context and `&styxrpc_scalar_module_tree` as the tree to
[styxrpc_server_create](server_create.md). The dispatcher allocates one state
per open fid. A function executes synchronously once per complete accepted
request on the dispatcher thread. Clunk and disconnect release the call state.
Keep the module and all borrowed descriptors alive and immutable through
server destruction. See [scalar calls](scalar_invoke.md).
