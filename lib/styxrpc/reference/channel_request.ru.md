# styxrpc_channel_request

[English](channel_request.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel.h>
StyxRpcStatus styxrpc_channel_request(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, void *scratch, size_t scratch_size,
    void *bytes, size_t capacity, size_t *used);
```

Кодирует одну отправку/приём либо до восьми веток ALT по порядку. Все ключи должны быть выданы таблицей полномочий. ALT использует отдельный scratch. Размер результата измеряется в байтах.

C99. `StyxRpc::rpc`.

[Пример](../tests/channel_socket.c) · [Исходник](../src/channel.c)
