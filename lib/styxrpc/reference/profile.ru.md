# styxrpc_profile

[English](profile.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/rpc.h>
StyxRpcStatus styxrpc_profile(const StyxRpcExport *exports, size_t count, void *bytes, size_t capacity, size_t *size);
```

Кодирует count разных описателей exports в переданный буфер bytes ёмкости capacity байт. При INVALID size равен нулю. Публикуйте ровно size байт в .interface.

C99. Вызывайте из диспетчера, сериализующего операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/device.c) · [Исходник](../src/rpc.c)
