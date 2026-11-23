# styxrpc_managed_write

[English](managed_write.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_write(StyxRpcManagedCall *call, size_t offset, const void *bytes, size_t size,
    void *scratch, size_t capacity);
```

Принимает size байт с очередного смещения offset. Ошибка кадра отклоняет fid. После полной проверки один раз вызывает begin. MORE обозначает часть кадра или отложенный ответ. Scratch не пересекается с другими буферами.

C99. Сериализуйте операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/adjust.c) · [Исходник](../src/managed.c)
