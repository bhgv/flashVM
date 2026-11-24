# styxrpc_module_destroy

[English](module_destroy.md) · [API](../include/styxrpc/module.h) · [Source](../src/module.c) · [Guide](../MODULES.ru.md)

`StyxRpc::serve`

```c
void styxrpc_module_destroy(StyxRpcModule *module);
```

## Контракт

Передайте созданный SDK образ или NULL. Вызов освобождает копии описателей и метаданных. Сначала закройте все серверы и их fid. Контексты, реестр каналов и описатель управления остаются у приложения.
