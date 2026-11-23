# styxrpc_managed_read

[English](managed_read.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_read(const StyxRpcManagedCall *call, size_t offset, void *bytes,
    size_t capacity, size_t *size);
```

Копирует до capacity байт ответа по смещению offset. MORE означает ожидание, OK и ноль байт — EOF. Память bytes и size принадлежит вызывающему.

C99. Сериализуйте операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/adjust.c) · [Исходник](../src/managed.c)
