# styxrpc_channel_fid_write

[English](channel_fid_write.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_fid.h>
StyxRpcStatus styxrpc_channel_fid_write(StyxRpcChannelFid *fid, size_t offset, const void *bytes, size_t size);
```

Принимает последовательный фрагмент в обнулённый настроенный fid. MORE означает неполный кадр. OK передаёт запрос планировщику единожды. Буфер запроса удерживается до закрытия.

C99. Операции общего fid сериализует его владелец. `StyxRpc::rpc`.

[Пример](../tests/channel_socket.c) · [Исходник](../src/channel_fid.c)
