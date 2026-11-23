# styxrpc_read

[English](read.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/rpc.h>
StyxRpcStatus styxrpc_read(const StyxRpcCall *call, size_t offset, void *bytes, size_t capacity, size_t *size);
```

Копирует из ответа call до capacity байт с позиции offset в bytes. При OK size содержит фактический объём, ноль в EOF. MORE означает, что ответа пока нет. INVALID означает ошибку аргумента или отклонённый вызов.

C99. Вызывайте из диспетчера, сериализующего операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/device.c) · [Исходник](../src/rpc.c)
