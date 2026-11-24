# styxrpc_module_slot_set

[Русский](module_slot_set.ru.md) · [module_slot.h](../include/styxrpc/module_slot.h) · [module_slot.c](../src/module_slot.c)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_module_slot_set(StyxRpcModuleSlot *slot, StyxRpcModule *module);
```

Pass a live slot and a borrowed compiled module. NULL withdraws the image. The call closes all image-specific open files before returning OK. It leaves stable controls untouched. INVALID means the slot is missing or the identity space is exhausted. Call on the server owner between requests or from the control callback. Keep the installed image until withdrawal or destruction.

[StyxRpcModuleSlot](module_slot.md)
