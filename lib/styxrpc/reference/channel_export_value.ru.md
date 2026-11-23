# styxrpc_channel_export_value

[English](channel_export_value.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_call.h>
StyxRpcStatus styxrpc_channel_export_value(const unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE],
    uint32_t key, int unbuffered, void *bytes, size_t capacity, size_t *used);
```

Кодирует непрозрачный токен аренды и положительный 31-битный ключ возвращаемого канала. unbuffered равен 0 или 1. Хост хранит реестр токенов, сверяет их на .unibus модуля и отзывает аренду при clunk функции.

`StyxRpc::rpc`.

[Пример](../examples/disvm/channel_device.c) · [Исходник](../src/channel_call.c)
