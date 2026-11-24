# styxrpc_control_destroy

[English](control_destroy.md) · [API](../include/styxrpc/control.h) · [Source](../src/control.c) · [Guide](../MODULES.ru.md)

`StyxRpc::serve`

```c
void styxrpc_control_destroy(StyxRpcControl *control);
```

## Контракт

Передайте свой описатель управления или NULL. Сначала закройте все серверы и их fid. Вызов освобождает описатель. Контекст и текущий программный образ принадлежат приложению.
