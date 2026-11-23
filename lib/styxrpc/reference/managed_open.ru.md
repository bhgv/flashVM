# styxrpc_managed_open

[English](managed_open.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_open(StyxRpcManagedCall *call, const StyxRpcManagedExport *exported,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity);
```

Инициализирует новое или закрытое состояние call с двумя непересекающимися буферами приложения. Ёмкость каждого 1–65536 байт. Описание и память живут до закрытия.

C99. Сериализуйте операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/adjust.c) · [Исходник](../src/managed.c)
