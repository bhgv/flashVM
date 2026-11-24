# styxrpc_module_destroy

[Русский](module_destroy.ru.md) · [API](../include/styxrpc/module.h) · [Source](../src/module.c) · [Guide](../MODULES.md)

`StyxRpc::serve`

```c
void styxrpc_module_destroy(StyxRpcModule *module);
```

## Contract

Pass the image created by the SDK, or NULL. This consumes copied descriptors and metadata. Retire all using servers and their fids first. Callback contexts, the channel registry and control descriptor remain the application’s responsibility.
