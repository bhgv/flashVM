# styxrpc_module_create

[English](module_create.md) · [API](../include/styxrpc/module.h) · [Source](../src/module.c) · [Guide](../MODULES.ru.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_module_create(const StyxRpcModuleSpec *spec, StyxRpcModule **out);
```

## Контракт

Передайте именованный описатель с одним–восемью экспортами, нулём–восемью ячейками, необязательным реестром каналов и управлением. Контексты колбэков, реестр и описатель управления живут до закрытия всех серверов. При успехе имена и описатели копируются в новый образ в `out`. Неверное объявление или ошибка выделения дают `STYXRPC_INVALID` и сохраняют `out`. Передайте образ как контекст `styxrpc_module_tree` в потоке владельца.
