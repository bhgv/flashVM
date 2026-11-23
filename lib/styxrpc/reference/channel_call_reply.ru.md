# styxrpc_channel_call_reply

[English](channel_call_reply.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_call.h>
StyxRpcStatus styxrpc_channel_call_reply(const StyxRpcSchema *schema, const StyxRpcChannelCall *call,
    const void *result, size_t result_size, const void *updates, size_t update_size,
    void *bytes, size_t capacity, size_t *used);
```

Проверяет результат и разреженные обновления по исходному проверенному вызову. Оба параметра содержат полные S-записи в отдельных от результата буферах.

`StyxRpc::rpc`.

[Пример](../examples/disvm/channel_device.c) · [Исходник](../src/channel_call.c)
