# styxrpc_managed_finish

[English](managed_finish.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_finish(StyxRpcManagedCall *call, const StyxRpcOutput *output, void *scratch, size_t capacity);
```

Завершает ожидающий fid, используя отдельный scratch ёмкостью capacity байт. NULL output публикует удалённую ошибку. Ошибочный output даёт удалённую ошибку, если её S-запись помещается. Повтор и вызов после close дают INVALID.

C99. Сериализуйте операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/../tests/managed.c) · [Исходник](../src/managed.c)
