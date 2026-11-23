# styxrpc_channel_fid_close

[English](channel_fid_close.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_fid.h>
void styxrpc_channel_fid_close(StyxRpcChannelFid *fid);
```

Закрывает операцию. Если она ждала, вызывает отмену планировщика один раз до повторного использования буферов. Согласуйте закрытие с планировщиком.

C99. Операции общего fid сериализует его владелец. `StyxRpc::rpc`.

[Пример](../tests/channel_socket.c) · [Исходник](../src/channel_fid.c)
