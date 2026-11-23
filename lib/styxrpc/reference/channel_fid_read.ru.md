# styxrpc_channel_fid_read

[English](channel_fid_read.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_fid.h>
StyxRpcStatus styxrpc_channel_fid_read(const StyxRpcChannelFid *fid, size_t offset,
    void *bytes, size_t capacity, size_t *size);
```

Читает готовый ответ с байтового смещения. MORE означает ожидание, OK и ноль байт — EOF. Согласуйте чтение с завершением и закрытием.

C99. Операции общего fid сериализует его владелец. `StyxRpc::rpc`.

[Пример](../tests/channel_socket.c) · [Исходник](../src/channel_fid.c)
