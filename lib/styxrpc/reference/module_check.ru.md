# styxrpc_module_check

[English](module_check.md) · [API](../include/styxrpc/module_client.h) · [Source](../src/client_exchange.c) · [Guide](../MODULES.ru.md)

`StyxRpc::launch`

```c
StyxRpcStatus styxrpc_module_check(struct StyxClient *session, const char *module,
    const StyxRpcModuleImport *entries, size_t count, uint32_t pid);
```

## Контракт

Заимствует смонтированную сессию и относительный обычный путь. Передайте от одного до восьми компилированных импортов в `entries`. Встречный `pid` задаёт доступ для профилей, которым он нужен. Вызов проверяет сигнатуры, схемы и маркеры в одном удерживаемом каталоге, используя сроки I/O сессии. Функции модуля не вызываются. Возвращает `STYXRPC_OK` при совпадении, `STYXRPC_INVALID` при ошибке описания, доступа или проверки.
