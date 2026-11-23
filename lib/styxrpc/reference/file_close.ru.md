# styxrpc_file_close

[English](file_close.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/vfs.h>
void styxrpc_file_close(StyxRpcFile *file);
```

Освобождает состояние колбэка один раз и обнуляет file. Допускает NULL и уже закрытое состояние. Диспетчер вызывает функцию при clunk или обрыве соединения.

C99. Вызывайте из диспетчера, сериализующего операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/device.c) · [Исходник](../src/vfs.c)
