# styxrpc_open

[English](open.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/rpc.h>
StyxRpcStatus styxrpc_open(StyxRpcCall *call, const StyxRpcExport *exported);
```

Инициализирует память call для одного открытого fid функции. Описатель exported должен жить до закрытия fid. При отсутствии колбэка, неверном имени или сигнатуре возвращает INVALID.

C99. Вызывайте из диспетчера, сериализующего операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/device.c) · [Исходник](../src/rpc.c)
