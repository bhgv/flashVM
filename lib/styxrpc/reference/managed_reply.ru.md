# styxrpc_managed_reply

[English](managed_reply.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_reply(uint32_t signature, const StyxRpcSchema *schema, const void *request,
    size_t request_size, const void *reply, size_t reply_size, StyxRpcManagedReply *result);
```

Проверяет два полных кадра и выбирает изменённые ref по исходному запросу. OK выдаёт представления в reply, REMOTE_ERROR — подтверждённую ошибку провайдера. Изменения применяет вызывающее приложение после проверки.

C99. Сериализуйте операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/../tests/managed.c) · [Исходник](../src/managed.c)
