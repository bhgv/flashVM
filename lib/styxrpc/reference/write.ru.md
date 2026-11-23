# styxrpc_write

[English](write.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/rpc.h>
StyxRpcStatus styxrpc_write(StyxRpcCall *call, size_t offset, const void *bytes, size_t size);
```

Принимает size байт запроса bytes по следующему смещению offset на call. MORE подтверждает часть кадра. OK подтверждает конец и один синхронный вызов колбэка. INVALID означает отказ при ошибке кадра или смещения. Операции с call сериализует вызывающий.

C99. Вызывайте из диспетчера, сериализующего операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/device.c) · [Исходник](../src/rpc.c)
