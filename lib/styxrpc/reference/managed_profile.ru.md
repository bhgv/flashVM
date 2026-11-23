# styxrpc_managed_profile

[English](managed_profile.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_profile(const StyxRpcManagedExport *exported,
    void *bytes, size_t capacity, size_t *size);
```

Кодирует один типизированный экспорт в bytes ёмкостью capacity байт. При успехе size задаёт длину записи для .interface. Колбэк остаётся локальным.

C99. Сериализуйте операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/adjust.c) · [Исходник](../src/managed.c)
