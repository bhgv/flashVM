# styxrpc_reply

[English](reply.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/rpc.h>
StyxRpcStatus styxrpc_reply(uint32_t signature, const void *bytes, size_t size, int64_t *value);
```

Разбирает ровно size полученных байт по signature. При INVALID или REMOTE_ERROR value не меняется. Перед разбором нужно получить весь ответ.

C99. Вызывайте из диспетчера, сериализующего операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/client.c) · [Исходник](../src/rpc.c)
