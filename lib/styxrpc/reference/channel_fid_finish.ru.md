# styxrpc_channel_fid_finish

[English](channel_fid_finish.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_fid.h>
StyxRpcStatus styxrpc_channel_fid_finish(StyxRpcChannelFid *fid, unsigned selected, const void *value, size_t size);
```

Публикует ответ на одном ожидающем fid. При INVALID из-за ошибки кодирования запрос остаётся ожидающим. Планировщик определяет готовность и фиксирует только одну ветку.

C99. Операции общего fid сериализует его владелец. `StyxRpc::rpc`.

[Пример](../tests/channel_socket.c) · [Исходник](../src/channel_fid.c)
