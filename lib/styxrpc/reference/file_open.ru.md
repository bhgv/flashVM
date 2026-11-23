# styxrpc_file_open

[English](file_open.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/vfs.h>
StyxRpcStatus styxrpc_file_open(StyxRpcFile *file, const StyxRpcVfs *ops, void *context, uint64_t node, unsigned access);
```

Связывает обнулённое состояние file с узлом node через ops и context. В access передайте STYXRPC_READ и STYXRPC_WRITE. При OK состояние колбэка живёт до закрытия. INVALID означает отказ или ошибку аргумента. Права узла до открытия проверяет диспетчер.

C99. Вызывайте из диспетчера, сериализующего операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/device.c) · [Исходник](../src/vfs.c)
