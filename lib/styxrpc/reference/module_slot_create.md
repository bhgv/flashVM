# styxrpc_module_slot_create

[Русский](module_slot_create.ru.md) · [module_slot.h](../include/styxrpc/module_slot.h) · [module_slot.c](../src/module_slot.c)

`StyxRpc::serve`

```c
StyxRpcModuleSlot *styxrpc_module_slot_create(StyxRpcControl *control);
```

Pass a live control tree. The slot borrows it and starts empty. Returns the owned slot or NULL for invalid control or allocation failure.

[StyxRpcModuleSlot](module_slot.md)
