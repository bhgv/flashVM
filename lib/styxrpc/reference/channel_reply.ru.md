# styxrpc_channel_reply

[English](channel_reply.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel.h>
StyxRpcStatus styxrpc_channel_reply(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, unsigned selected, const void *value, size_t size,
    void *bytes, size_t capacity, size_t *used);
```

Кодирует выбранную ветку с единицы, отсутствие готовой ветки (0) либо ошибку (255). Ноль допустим только для try и ограниченного ожидания. Типизированный приём включает кадр элемента. Буферы не пересекаются.

C99. `StyxRpc::rpc`.

[Пример](../tests/channel_socket.c) · [Исходник](../src/channel.c)
