# styxrpc_channel_read

[English](channel_read.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel.h>
StyxRpcStatus styxrpc_channel_read(const StyxRpcChannelGrant *grants, size_t grant_count,
    const void *bytes, size_t size, StyxRpcSelection *operation);
```

Декодирует полную операцию, проверяет ключи и типы элементов и выдаёт заимствованные ветки. Храните запрос до завершения работы с ними.

C99. `StyxRpc::rpc`.

[Пример](../tests/channel_socket.c) · [Исходник](../src/channel.c)
