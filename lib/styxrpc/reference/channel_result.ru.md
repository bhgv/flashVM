# styxrpc_channel_result

[English](channel_result.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel.h>
StyxRpcStatus styxrpc_channel_result(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, const void *bytes, size_t size, StyxRpcChannelResult *result);
```

Проверяет точный ответ по исходной операции и полномочиям. При успехе выдаёт номер и int либо заимствованный типизированный кадр. REMOTE_ERROR означает отказ участника.

C99. `StyxRpc::rpc`.

[Пример](../tests/channel_socket.c) · [Исходник](../src/channel.c)
