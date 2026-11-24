# styxrpc_module_slot_destroy

[English](module_slot_destroy.md) · [module_slot.h](../include/styxrpc/module_slot.h) · [module_slot.c](../src/module_slot.c)

`StyxRpc::serve`

```c
void styxrpc_module_slot_destroy(StyxRpcModuleSlot *slot);
```

Освободите свой слот после закрытия всех fid сервером. NULL допустим. Управление и последний установленный образ остаются у вызывающего.

[StyxRpcModuleSlot](module_slot.ru.md)
