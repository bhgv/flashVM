# styxrpc_file_write

[English](file_write.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/vfs.h>
StyxRpcStatus styxrpc_file_write(StyxRpcFile *file, uint64_t offset, const void *bytes, size_t count, size_t *size);
```

Передаёт count байт из bytes в file со смещения offset. При OK размер size подтверждённой записи может быть меньше count. INVALID означает отсутствие права записи, ошибку аргумента или отказ колбэка.

C99. Вызывайте из диспетчера, сериализующего операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/device.c) · [Исходник](../src/vfs.c)
