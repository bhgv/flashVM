# styxrpc_channel_profile

[English](channel_profile.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_call.h>
StyxRpcStatus styxrpc_channel_profile(const char *name, uint32_t signature, const StyxRpcSchema *schema,
    void *bytes, size_t capacity, size_t *used);
```

Записывает контекстную запись .interface по сигнатуре Limbo. Опубликуйте .unibus-version и позиционный маркер .ref-fields модуля. Подключайте по двустороннему Styx.

`StyxRpc::rpc`.

[Пример](../examples/disvm/channel_device.c) · [Исходник](../src/channel_call.c)
