# styxrpc_module_create

[Русский](module_create.ru.md) · [API](../include/styxrpc/module.h) · [Source](../src/module.c) · [Guide](../MODULES.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_module_create(const StyxRpcModuleSpec *spec, StyxRpcModule **out);
```

## Contract

Pass a named spec with one to eight exports, zero to eight variable cells, an optional channel registry and optional control files. Keep callback contexts, the registry and control descriptor alive until every using server has retired. A successful call copies names and descriptors to the new image in `out`. Invalid declarations or allocation failure return `STYXRPC_INVALID` and preserve `out`. Serve the image as the context of `styxrpc_module_tree` on an owner thread.
