# styxrpc_module_slot_create

[English](module_slot_create.md) · [module_slot.h](../include/styxrpc/module_slot.h) · [module_slot.c](../src/module_slot.c)

`StyxRpc::serve`

```c
StyxRpcModuleSlot *styxrpc_module_slot_create(StyxRpcControl *control);
```

Передайте живое дерево управления. Слот заимствует control и начинается пустым. Возвращает свой слот либо NULL при неверном control или нехватке памяти.

[StyxRpcModuleSlot](module_slot.ru.md)
