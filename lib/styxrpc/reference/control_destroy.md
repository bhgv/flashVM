# styxrpc_control_destroy

[Русский](control_destroy.ru.md) · [API](../include/styxrpc/control.h) · [Source](../src/control.c) · [Guide](../MODULES.md)

`StyxRpc::serve`

```c
void styxrpc_control_destroy(StyxRpcControl *control);
```

## Contract

Pass the owned control descriptor or NULL. Close all using servers and their fids first. This operation frees the descriptor. The application owns its callback context and current program image.
