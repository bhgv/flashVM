# styxrpc_module_slot_destroy

[Русский](module_slot_destroy.ru.md) · [module_slot.h](../include/styxrpc/module_slot.h) · [module_slot.c](../src/module_slot.c)

`StyxRpc::serve`

```c
void styxrpc_module_slot_destroy(StyxRpcModuleSlot *slot);
```

Consume the owned slot after the server has closed all held fids. NULL is accepted. The control and last installed image remain owned by the caller.

[StyxRpcModuleSlot](module_slot.md)
