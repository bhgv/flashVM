# styxrpc_file_read

[English](file_read.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/vfs.h>
StyxRpcStatus styxrpc_file_read(StyxRpcFile *file, uint64_t offset, void *bytes, size_t capacity, size_t *size);
```

Читает из file со смещения offset в переданный буфер bytes до capacity байт. OK сообщает подтверждённый size, ноль в EOF. INVALID означает отсутствие права чтения, ошибку буфера или отказ колбэка.

C99. Вызывайте из диспетчера, сериализующего операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/device.c) · [Исходник](../src/vfs.c)
