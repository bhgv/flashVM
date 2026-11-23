# styxrpc_channel_call

[English](channel_call.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_call.h>
StyxRpcStatus styxrpc_channel_call(const StyxRpcSchema *schema, const void *bytes, size_t size,
    StyxRpcChannelCall *call);
```

Читает полную контекстную запись графа и извлекает первый PID и полномочия каналов. Исходный буфер живёт до конца вызова. Псевдонимы обязаны иметь общий тип элементов.

`StyxRpc::rpc`.

[Пример](../examples/disvm/channel_device.c) · [Исходник](../src/channel_call.c)
