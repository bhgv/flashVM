# styxrpc_module_slot_set

[English](module_slot_set.md) · [module_slot.h](../include/styxrpc/module_slot.h) · [module_slot.c](../src/module_slot.c)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_module_slot_set(StyxRpcModuleSlot *slot, StyxRpcModule *module);
```

Передайте живой слот и заимствованный собранный образ. NULL отзывает публикацию. До возврата OK вызов закрывает все открытые файлы образа. Файлы управления сохраняются. INVALID означает отсутствие слота или исчерпание идентификаторов. Вызывайте у владельца между запросами либо из колбэка управления. Удерживайте образ до отзыва или уничтожения.

[StyxRpcModuleSlot](module_slot.ru.md)
